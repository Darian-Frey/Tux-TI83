import QtQuick
import QtQuick.Layouts
import ".."

// Graph rendering for the active function buffers (Y1 / Y2 / Y3).
//
// Visual contract: a dark Rectangle (LCD-style) containing a Canvas
// that draws axis labels, gridlines, and up to three function curves.
// Tick step is computed from the current viewport range. Colours are
// pulled from the Style singleton so the curves echo the calculator's
// semantic palette.
//
// Behavioural contract:
//   - Mouse drag pans the viewport via `uiController.pan()`
//   - Mouse wheel zooms via `uiController.zoom()`
//   - Repaints automatically on viewportChanged / displayChanged /
//     activeFunctionIndexChanged signals from the controller
//
// Limitations:
//   - Curve-to-Y-slot colour mapping is based on the result index
//     returned by `getMultiGraphPoints()`, which currently skips empty
//     function slots. Means colours can shift when slots are
//     non-contiguous. Tracked as BUG-012.
Rectangle {
    id: root

    Layout.fillWidth: true
    Layout.fillHeight: true

    color: Style.bgDisplay
    radius: 4
    border.color: Style.bgSection
    border.width: 1
    clip: true

    // ── Plot draw animation (MODE → Plot: Sequential/Simul) ──
    // `reveal` < 0 means "fully drawn" (steady state). An animation counts
    // it up from 0: Simul → points-per-curve; Sequential → a running index
    // across the concatenated curves. `plotData` caches the computed points
    // so animation frames don't recompute the whole plot; `animMax` is the
    // reveal value at which the sweep completes.
    property int reveal: -1
    property var plotData: []
    property int animMax: 0

    // Refresh the cached plot points and repaint at the current reveal
    // state (no animation) — used for pan/zoom/trace and other live updates.
    function refreshPlot() {
        plotData = uiController.getMultiGraphPoints(600)
        canvas.requestPaint()
    }

    // Kick off a fresh draw animation (entering the graph, function edits,
    // graph-mode switch). Computes the sweep length from the cached points.
    function startPlotAnim() {
        plotTimer.stop()
        plotData = uiController.getMultiGraphPoints(600)
        var maxLen = 0, total = 0
        for (var i = 0; i < plotData.length; i++) {
            var n = plotData[i] ? plotData[i].length : 0
            if (n > maxLen) maxLen = n
            total += n
        }
        animMax = (uiController.plotMode === 1) ? maxLen : total
        if (animMax <= 1) { reveal = -1; canvas.requestPaint(); return }
        reveal = 0
        plotTimer.start()
        canvas.requestPaint()
    }

    Component.onCompleted: refreshPlot()

    // ~25 frames → ~0.4s sweep regardless of point count.
    Timer {
        id: plotTimer
        interval: 16
        repeat: true
        running: false
        onTriggered: {
            root.reveal += Math.max(1, Math.ceil(root.animMax / 25))
            if (root.reveal >= root.animMax) {
                root.reveal = -1        // done → steady full-draw state
                plotTimer.stop()
            }
            canvas.requestPaint()
        }
    }

    // Animate the draw when the graph becomes visible (GRAPH key switches
    // the StackLayout to this page).
    onVisibleChanged: if (visible) startPlotAnim()

    Canvas {
        id: canvas
        anchors.fill: parent

        // Curve colours come from Style.graphColors (10 slots, Y1..Y0).

        onPaint: {
            const ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)

            const xMin = uiController.xMin
            const xMax = uiController.xMax
            const yMin = uiController.yMin
            const yMax = uiController.yMax
            const rangeX = xMax - xMin
            const rangeY = yMax - yMin
            if (rangeX <= 0 || rangeY <= 0) return

            function toPx(x, y) {
                return {
                    x: (x - xMin) * (width / rangeX),
                    y: height - (y - yMin) * (height / rangeY)
                }
            }

            // Tick spacing comes from Xscl/Yscl (the axis tick / grid-line
            // interval). Guard against zero, negative, or absurdly dense
            // values — which would freeze the paint loop — by falling back
            // to a zoom-based heuristic (max ~400 divisions per axis).
            const heuristicX = rangeX > 50 ? 10 : (rangeX < 5 ? 0.5 : 1)
            const heuristicY = rangeY > 50 ? 10 : (rangeY < 5 ? 0.5 : 1)
            let xStep = uiController.xScl
            let yStep = uiController.yScl
            if (!(xStep > 0) || rangeX / xStep > 400) xStep = heuristicX
            if (!(yStep > 0) || rangeY / yStep > 400) yStep = heuristicY

            ctx.font = "10px " + Style.monoFamily
            ctx.fillStyle = Style.textMuted

            // FORMAT flags (2ND+ZOOM): gate grid lines, axes, and labels.
            const gridOn = uiController.gridOn
            const axesOn = uiController.axesOn
            const labelOn = uiController.labelOn

            // Axis pixel positions (clamped on-screen) for the tick marks.
            const axisY = toPx(0, 0).y
            const axisX = toPx(0, 0).x
            const TICK = 3  // half-length of an axis tick mark, px

            // Vertical grid lines + x-axis labels + ticks. The x≈0 line is
            // the (y-)axis; others are grid at Xscl intervals.
            for (let x = Math.floor(xMin / xStep) * xStep; x <= xMax; x += xStep) {
                const px = toPx(x, 0)
                const isAxis = Math.abs(x) < xStep * 0.001
                if (isAxis ? axesOn : gridOn) {
                    ctx.strokeStyle = isAxis ? Style.textMuted : Style.gridLine
                    ctx.beginPath()
                    ctx.moveTo(px.x, 0)
                    ctx.lineTo(px.x, height)
                    ctx.stroke()
                }
                // Tick mark on the x-axis at each Xscl step.
                if (!isAxis && axesOn) {
                    ctx.strokeStyle = Style.textMuted
                    ctx.beginPath()
                    ctx.moveTo(px.x, axisY - TICK)
                    ctx.lineTo(px.x, axisY + TICK)
                    ctx.stroke()
                }
                if (!isAxis && labelOn) {
                    ctx.fillText(x.toFixed(1), px.x + 2, height - 5)
                }
            }

            // Horizontal grid lines + y-axis labels + ticks.
            for (let y = Math.floor(yMin / yStep) * yStep; y <= yMax; y += yStep) {
                const py = toPx(0, y)
                const isAxis = Math.abs(y) < yStep * 0.001
                if (isAxis ? axesOn : gridOn) {
                    ctx.strokeStyle = isAxis ? Style.textMuted : Style.gridLine
                    ctx.beginPath()
                    ctx.moveTo(0, py.y)
                    ctx.lineTo(width, py.y)
                    ctx.stroke()
                }
                // Tick mark on the y-axis at each Yscl step.
                if (!isAxis && axesOn) {
                    ctx.strokeStyle = Style.textMuted
                    ctx.beginPath()
                    ctx.moveTo(axisX - TICK, py.y)
                    ctx.lineTo(axisX + TICK, py.y)
                    ctx.stroke()
                }
                if (!isAxis && labelOn) {
                    ctx.fillText(y.toFixed(1), 5, py.y - 2)
                }
            }

            // Trace cursor + X / Y readout. Drawn AFTER the curves so
            // the marker sits on top. Active function index picks both
            // the curve evaluated for the readout and the colour of
            // the marker so it visually ties to its source curve.
            const traceActive = uiController.isTracing && uiController.isGraphMode
            // Reserved for the post-curve marker pass below.

            // Function curves. Connected mode (default) draws line
            // segments between adjacent samples; Dot mode draws one
            // filled circle per sample with no connecting strokes —
            // matches TI-83 MODE → Connected/Dot.
            // Function curves. The global MODE → Dot draws every curve as
            // dots; otherwise each slot uses its Y-editor line style
            // (0 thin / 1 thick / 2 dotted).
            // Cached plot points (refreshed on every non-animation repaint
            // trigger; frozen during the draw animation). Fall back to a
            // fresh fetch if the cache hasn't been primed yet.
            const multiPts = (root.plotData && root.plotData.length)
                             ? root.plotData
                             : uiController.getMultiGraphPoints(600)
            const dotMode = uiController.drawMode === 1
            // Plot draw animation (MODE → Plot). reveal < 0 → draw everything
            // (steady state). Simul reveals `reveal` points of every curve;
            // Sequential reveals along the concatenated curves in order.
            const simul = uiController.plotMode === 1

            // Inequality shading (Inequalz-style). Skipped during the plot
            // draw-animation (reveal >= 0) to avoid a half-swept polygon.
            if (root.reveal < 0) {
                if (uiController.shadeMode === 1) {
                    // Intersection: the controller returns band segments
                    // covering the region where ALL relational inequalities
                    // hold. Fill each as one translucent polygon.
                    const segs = uiController.getInequalityShade(600)
                    if (segs.length > 0) {
                        ctx.fillStyle = Style.textResult
                        ctx.globalAlpha = 0.26
                        for (let s = 0; s < segs.length; s++) {
                            const seg = segs[s]
                            ctx.beginPath()
                            for (let i = 0; i < seg.length; i++) {
                                const p = toPx(seg[i].x, seg[i].top)
                                if (i === 0) ctx.moveTo(p.x, p.y)
                                else ctx.lineTo(p.x, p.y)
                            }
                            for (let i = seg.length - 1; i >= 0; i--) {
                                const p = toPx(seg[i].x, seg[i].bottom)
                                ctx.lineTo(p.x, p.y)
                            }
                            ctx.closePath()
                            ctx.fill()
                        }
                        ctx.globalAlpha = 1.0
                    }
                } else {
                    // Union: each slot with a relation fills the region below
                    // (< / ≤) or above (> / ≥) its own curve, independently.
                    for (let g = 0; g < multiPts.length; g++) {
                        const rel = uiController.functionRelation(g)
                        if (rel === 0) continue
                        const gpts = multiPts[g]
                        if (!gpts || gpts.length < 2) continue
                        const below = (rel === 1 || rel === 3)  // < or ≤
                        const edgeY = below ? height : 0
                        ctx.fillStyle = Style.graphColors[g % Style.graphColors.length]
                        ctx.globalAlpha = 0.18
                        ctx.beginPath()
                        const g0 = toPx(gpts[0].x, gpts[0].y)
                        ctx.moveTo(g0.x, g0.y)
                        for (let i = 1; i < gpts.length; i++) {
                            const gp = toPx(gpts[i].x, gpts[i].y)
                            ctx.lineTo(gp.x, gp.y)
                        }
                        const gl = toPx(gpts[gpts.length - 1].x, gpts[gpts.length - 1].y)
                        ctx.lineTo(gl.x, edgeY)
                        ctx.lineTo(g0.x, edgeY)
                        ctx.closePath()
                        ctx.fill()
                        ctx.globalAlpha = 1.0
                    }
                }
            }

            let priorTotal = 0
            for (let f = 0; f < multiPts.length; f++) {
                const pts = multiPts[f]
                const len = pts ? pts.length : 0
                let drawCount = len
                if (root.reveal >= 0) {
                    drawCount = simul
                        ? Math.min(root.reveal, len)
                        : Math.max(0, Math.min(len, root.reveal - priorTotal))
                }
                priorTotal += len
                if (len === 0 || drawCount === 0) continue
                const colour = Style.graphColors[f % Style.graphColors.length]
                const fstyle = uiController.functionStyle(f)
                if (dotMode) {
                    ctx.fillStyle = colour
                    for (let i = 0; i < drawCount; i++) {
                        const p = toPx(pts[i].x, pts[i].y)
                        ctx.beginPath()
                        ctx.arc(p.x, p.y, 1.5, 0, 2 * Math.PI)
                        ctx.fill()
                    }
                } else {
                    ctx.beginPath()
                    ctx.strokeStyle = colour
                    ctx.lineWidth = (fstyle === 1) ? 4 : 2  // thick vs thin
                    ctx.setLineDash(fstyle === 2 ? [2, 4] : [])  // dotted
                    for (let i = 0; i < drawCount; i++) {
                        const p = toPx(pts[i].x, pts[i].y)
                        if (i === 0) ctx.moveTo(p.x, p.y)
                        else ctx.lineTo(p.x, p.y)
                    }
                    ctx.stroke()
                    ctx.setLineDash([])
                }
            }

            // Stat plot (Phase C Wave 5b) — drawn over the function
            // curves, under the trace marker. Rendered in the result-green
            // accent so it reads as data rather than a function curve.
            const sp = uiController.getStatPlotData()
            if (sp.on && !sp.error) {
                const spCol = Style.textResult
                if (sp.type === 0 || sp.type === 1) {
                    const pts = sp.points
                    if (sp.type === 1 && pts.length > 1) {
                        // xyLine — connect points (already x-sorted)
                        ctx.beginPath()
                        ctx.strokeStyle = spCol
                        ctx.lineWidth = 1.5
                        for (let i = 0; i < pts.length; i++) {
                            const p = toPx(pts[i].x, pts[i].y)
                            if (i === 0) ctx.moveTo(p.x, p.y)
                            else ctx.lineTo(p.x, p.y)
                        }
                        ctx.stroke()
                    }
                    // markers (small filled squares)
                    ctx.fillStyle = spCol
                    for (let i = 0; i < pts.length; i++) {
                        const p = toPx(pts[i].x, pts[i].y)
                        ctx.fillRect(p.x - 2, p.y - 2, 4, 4)
                    }
                } else if (sp.type === 2) {
                    // histogram — bars, frequency (count) on the y-axis
                    ctx.fillStyle = spCol
                    ctx.strokeStyle = Style.lcdOverlay
                    ctx.lineWidth = 1
                    for (let i = 0; i < sp.bins.length; i++) {
                        const b = sp.bins[i]
                        const p0 = toPx(b.lo, 0)
                        const p1 = toPx(b.hi, b.count)
                        const bx = Math.min(p0.x, p1.x)
                        const bw = Math.abs(p1.x - p0.x)
                        const by = Math.min(p0.y, p1.y)
                        const bhh = Math.abs(p0.y - p1.y)
                        if (b.count > 0) {
                            ctx.fillRect(bx, by, bw, bhh)
                            ctx.strokeRect(bx, by, bw, bhh)
                        }
                    }
                } else if (sp.type === 3) {
                    // box plot at a fixed screen y (1-D — only x matters)
                    const b = sp.box
                    const yc = height * 0.5
                    const bh = 24
                    const xMinP = toPx(b.min, 0).x
                    const xMaxP = toPx(b.max, 0).x
                    const q1P = toPx(b.q1, 0).x
                    const q3P = toPx(b.q3, 0).x
                    const medP = toPx(b.med, 0).x
                    ctx.strokeStyle = spCol
                    ctx.lineWidth = 1.5
                    ctx.beginPath()
                    // whiskers + caps
                    ctx.moveTo(xMinP, yc); ctx.lineTo(q1P, yc)
                    ctx.moveTo(q3P, yc); ctx.lineTo(xMaxP, yc)
                    ctx.moveTo(xMinP, yc - 8); ctx.lineTo(xMinP, yc + 8)
                    ctx.moveTo(xMaxP, yc - 8); ctx.lineTo(xMaxP, yc + 8)
                    ctx.stroke()
                    // box + median
                    ctx.strokeRect(q1P, yc - bh / 2, q3P - q1P, bh)
                    ctx.beginPath()
                    ctx.moveTo(medP, yc - bh / 2); ctx.lineTo(medP, yc + bh / 2)
                    ctx.stroke()
                }
            }

            // DRAW-menu overlays (Phase D) — persistent shapes over the
            // curves, in a neutral light colour. Circles are drawn as a
            // 60-point polygon so they respect both axis scales (a true
            // circle in data coords) rather than an on-screen ellipse.
            const draws = uiController.getDrawObjects()
            if (draws.length > 0) {
                ctx.strokeStyle = Style.textDisplay
                ctx.fillStyle = Style.textDisplay
                ctx.lineWidth = 1.5
                ctx.setLineDash([])
                for (let d = 0; d < draws.length; d++) {
                    const o = draws[d]
                    if (o.type === "line") {
                        const p1 = toPx(o.a, o.b), p2 = toPx(o.c, o.d)
                        ctx.beginPath(); ctx.moveTo(p1.x, p1.y); ctx.lineTo(p2.x, p2.y); ctx.stroke()
                    } else if (o.type === "hline") {
                        const py = toPx(0, o.a).y
                        ctx.beginPath(); ctx.moveTo(0, py); ctx.lineTo(width, py); ctx.stroke()
                    } else if (o.type === "vline") {
                        const px = toPx(o.a, 0).x
                        ctx.beginPath(); ctx.moveTo(px, 0); ctx.lineTo(px, height); ctx.stroke()
                    } else if (o.type === "circle") {
                        ctx.beginPath()
                        for (let k = 0; k <= 60; k++) {
                            const t = k / 60 * 2 * Math.PI
                            const p = toPx(o.a + o.c * Math.cos(t), o.b + o.c * Math.sin(t))
                            if (k === 0) ctx.moveTo(p.x, p.y); else ctx.lineTo(p.x, p.y)
                        }
                        ctx.stroke()
                    } else if (o.type === "point") {
                        const p = toPx(o.a, o.b)
                        ctx.fillRect(p.x - 2, p.y - 2, 4, 4)
                    } else if (o.type === "text") {
                        const p = toPx(o.a, o.b)
                        ctx.font = "11px " + Style.monoFamily
                        ctx.fillText(o.text, p.x + 2, p.y - 2)
                    } else if (o.type === "curve") {
                        // DrawF — a sampled f(X) polyline.
                        ctx.beginPath()
                        for (let k = 0; k < o.pts.length; k++) {
                            const p = toPx(o.pts[k].x, o.pts[k].y)
                            if (k === 0) ctx.moveTo(p.x, p.y); else ctx.lineTo(p.x, p.y)
                        }
                        ctx.stroke()
                    } else if (o.type === "shade") {
                        // Shade( — fill between two sampled curves (upper
                        // forward, lower back), then outline both edges.
                        if (o.upper.length > 0 && o.lower.length > 0) {
                            ctx.globalAlpha = 0.28
                            ctx.beginPath()
                            for (let u = 0; u < o.upper.length; u++) {
                                const p = toPx(o.upper[u].x, o.upper[u].y)
                                if (u === 0) ctx.moveTo(p.x, p.y); else ctx.lineTo(p.x, p.y)
                            }
                            for (let w = o.lower.length - 1; w >= 0; w--) {
                                const p = toPx(o.lower[w].x, o.lower[w].y)
                                ctx.lineTo(p.x, p.y)
                            }
                            ctx.closePath(); ctx.fill()
                            ctx.globalAlpha = 1.0
                        }
                    }
                }
            }

            // Pixels (P7) — a 63×95 screen grid mapped across the canvas.
            const px = uiController.getPixels()
            if (px.length > 0) {
                ctx.fillStyle = Style.textDisplay
                const cw = width / 95, ch = height / 63
                for (let q = 0; q < px.length; q++) {
                    ctx.fillRect(Math.round(px[q].col * cw), Math.round(px[q].row * ch),
                                 Math.ceil(cw), Math.ceil(ch))
                }
            }

            // Trace marker + readout (drawn last so it sits on top).
            if (traceActive) {
                const tx = uiController.traceX
                const ty = uiController.traceY
                const activeIdx = uiController.activeFunctionIndex
                const traceColour = Style.graphColors[activeIdx % Style.graphColors.length]

                // Marker — only if traceY is finite and on-screen.
                if (isFinite(ty)) {
                    const m = toPx(tx, ty)
                    // Crosshair: small vertical + horizontal stroke
                    // through the point, plus a filled dot at the centre.
                    ctx.strokeStyle = traceColour
                    ctx.lineWidth = 1
                    ctx.beginPath()
                    ctx.moveTo(m.x, m.y - 8)
                    ctx.lineTo(m.x, m.y + 8)
                    ctx.moveTo(m.x - 8, m.y)
                    ctx.lineTo(m.x + 8, m.y)
                    ctx.stroke()
                    ctx.fillStyle = traceColour
                    ctx.beginPath()
                    ctx.arc(m.x, m.y, 2.5, 0, 2 * Math.PI)
                    ctx.fill()
                }

                // Readout — bottom-left of the canvas, on a translucent
                // strip so the digits stay legible regardless of what
                // the curve is doing under them.
                // Route both numbers through the controller's
                // formatScalar so the readout respects the user's
                // MODE Notation (Normal/Sci/Eng) and Decimal (Float
                // /Fix N) settings.
                // Expression strip (top-left) — FORMAT → Expr (ExprOn).
                // Shows the traced function's label and body, e.g. "Y1=X²".
                if (uiController.exprOn) {
                    const expr = uiController.functionLabel(activeIdx) +
                                 "=" + uiController.functionBufferText(activeIdx)
                    ctx.font = "11px " + Style.monoFamily
                    const ew = ctx.measureText(expr).width
                    ctx.fillStyle = Style.lcdOverlay
                    ctx.globalAlpha = 0.85
                    ctx.fillRect(0, 0, ew + 12, 18)
                    ctx.globalAlpha = 1.0
                    ctx.fillStyle = traceColour
                    ctx.fillText(expr, 6, 13)
                }

                // Coordinate readout (bottom-left) — gated by FORMAT → Coord.
                // Formatted per FORMAT → GC: RectGC shows X/Y, PolarGC shows
                // R/θ (θ in the current angle unit). The crosshair always
                // shows; only the numbers hide.
                if (uiController.coordOn) {
                    let readout
                    if (!isFinite(ty)) {
                        readout = (uiController.coordMode === 1)
                                  ? "R=—  θ=—" : "X=" + uiController.formatScalar(tx) + "  Y=—"
                    } else if (uiController.coordMode === 1) {
                        const r = Math.sqrt(tx * tx + ty * ty)
                        let theta = Math.atan2(ty, tx)
                        if (uiController.angleMode === 1) theta = theta * 180 / Math.PI
                        readout = "R=" + uiController.formatScalar(r) +
                                  "  θ=" + uiController.formatScalar(theta)
                    } else {
                        readout = "X=" + uiController.formatScalar(tx) +
                                  "  Y=" + uiController.formatScalar(ty)
                    }
                    if (uiController.poiTrace)
                        readout = "POI  " + readout
                    ctx.font = "11px " + Style.monoFamily
                    const textW = ctx.measureText(readout).width
                    const padX = 6, padY = 4
                    const rectW = textW + padX * 2
                    const rectH = 18
                    ctx.fillStyle = Style.lcdOverlay
                    ctx.globalAlpha = 0.85
                    ctx.fillRect(0, height - rectH, rectW, rectH)
                    ctx.globalAlpha = 1.0
                    ctx.fillStyle = traceColour
                    ctx.fillText(readout, padX, height - padY - 1)
                }
            }

            // ZBox rubber-band rectangle (drawn last, on top).
            if (dragArea.boxing) {
                const rx = Math.min(dragArea.boxX0, dragArea.boxX1)
                const ry = Math.min(dragArea.boxY0, dragArea.boxY1)
                const rw = Math.abs(dragArea.boxX1 - dragArea.boxX0)
                const rh = Math.abs(dragArea.boxY1 - dragArea.boxY0)
                ctx.fillStyle = Style.textExpr
                ctx.globalAlpha = 0.15
                ctx.fillRect(rx, ry, rw, rh)
                ctx.globalAlpha = 1.0
                ctx.strokeStyle = Style.textExpr
                ctx.lineWidth = 1
                ctx.strokeRect(rx, ry, rw, rh)
            }
        }

        MouseArea {
            id: dragArea
            anchors.fill: parent
            property real lastX: 0
            property real lastY: 0
            // ZBox rubber-band state (active while uiController.zoomBoxArm).
            property bool boxing: false
            property real boxX0: 0
            property real boxY0: 0
            property real boxX1: 0
            property real boxY1: 0

            // Pixel → data-coordinate conversion (inverse of toPx).
            function dataX(px) { return uiController.xMin + px / width * (uiController.xMax - uiController.xMin) }
            function dataY(py) { return uiController.yMin + (height - py) / height * (uiController.yMax - uiController.yMin) }

            onPressed: (mouse) => {
                if (uiController.zoomBoxArm) {
                    boxing = true
                    boxX0 = boxX1 = mouse.x
                    boxY0 = boxY1 = mouse.y
                } else {
                    lastX = mouse.x
                    lastY = mouse.y
                }
            }
            onPositionChanged: (mouse) => {
                if (boxing) {
                    boxX1 = mouse.x
                    boxY1 = mouse.y
                    canvas.requestPaint()
                } else if (pressed) {
                    uiController.pan(mouse.x - lastX, mouse.y - lastY, width, height)
                    lastX = mouse.x
                    lastY = mouse.y
                }
            }
            onReleased: (mouse) => {
                if (boxing) {
                    boxing = false
                    uiController.zoomBox(dataX(boxX0), dataY(boxY0),
                                         dataX(mouse.x), dataY(mouse.y))
                }
            }
            onWheel: (wheel) => {
                const factor = wheel.angleDelta.y > 0 ? 0.9 : 1.1
                uiController.zoom(factor, wheel.x, wheel.y, width, height)
            }
        }

        // Repaint on relevant controller signals. Point-set changes refresh
        // the cache; discrete graph events (re)start the draw animation;
        // pure overlay/interaction updates just repaint at the current state.
        Connections {
            target: uiController
            // Pan / zoom / window edits: recompute points, no animation.
            function onViewportChanged() { root.refreshPlot() }
            function onParamWindowChanged() { root.refreshPlot() }
            // Discrete graph events: sweep the draw animation.
            function onFunctionsChanged() { root.startPlotAnim() }
            function onGraphModeSettingChanged() { root.startPlotAnim() }
            function onPlotModeChanged() { root.startPlotAnim() }
            // Overlays / interaction: repaint at the current reveal state.
            function onDisplayChanged() { canvas.requestPaint() }
            function onActiveFunctionIndexChanged() { canvas.requestPaint() }
            function onDrawModeChanged() { canvas.requestPaint() }
            function onStatPlotChanged() { canvas.requestPaint() }
            function onFormatChanged() { canvas.requestPaint() }
            function onDrawObjectsChanged() { canvas.requestPaint() }
            function onTraceChanged() { canvas.requestPaint() }
            function onThemeChanged() { canvas.requestPaint() }
        }
    }

    // ✕ CLR — clear all overlays (program- or DRAW-menu-drawn). Shown only
    // when some exist, so it stays out of the way otherwise (P6-2).
    Rectangle {
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 6
        visible: uiController.drawObjectCount > 0
        width: clrLabel.width + 14
        height: 24
        radius: 4
        opacity: clrArea.containsMouse ? 1.0 : 0.85
        color: Style.bgSurface
        border.color: Style.textError
        border.width: 1
        Text {
            id: clrLabel
            anchors.centerIn: parent
            text: "✕ CLR"
            color: Style.textError
            font.family: Style.monoFamily
            font.pixelSize: Style.funcKeyLabelPixelSize
        }
        MouseArea {
            id: clrArea
            anchors.fill: parent
            hoverEnabled: true
            onClicked: uiController.clrDraw()
        }
    }

    // POI toggle — point-of-intersection trace. Shown only while tracing in
    // Func mode; when on, ←/→ jump between intersections of the active curve
    // and the other enabled curves. Sits top-left, clear of the ✕ CLR button.
    Rectangle {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.margins: 6
        visible: uiController.isTracing && uiController.graphMode === 0
        width: poiLabel.width + 14
        height: 24
        radius: 4
        opacity: poiArea.containsMouse ? 1.0 : 0.85
        color: uiController.poiTrace ? Style.opBg : Style.bgSurface
        border.color: uiController.poiTrace ? Style.textExpr : Style.keyBorderNeutral
        border.width: 1
        Text {
            id: poiLabel
            anchors.centerIn: parent
            text: "◆ POI"
            color: uiController.poiTrace ? Style.textExpr : Style.textSecondary
            font.family: Style.monoFamily
            font.pixelSize: Style.funcKeyLabelPixelSize
        }
        MouseArea {
            id: poiArea
            anchors.fill: parent
            hoverEnabled: true
            onClicked: uiController.togglePoiTrace()
        }
    }
}
