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

    Canvas {
        id: canvas
        anchors.fill: parent

        // Function-curve palette. Three slots match Y1/Y2/Y3 in the
        // common case (all three functions defined and contiguous).
        readonly property var fnColors: [Style.textExpr, Style.textError, Style.textResult]

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

            // Tick-step heuristic: keeps grid lines visually sane across
            // a range of zoom levels without computing log10.
            const step = rangeX > 50 ? 10 : (rangeX < 5 ? 0.5 : 1)

            ctx.font = "10px " + Style.monoFamily
            ctx.fillStyle = Style.textMuted

            // Vertical grid lines + x-axis labels.
            for (let x = Math.floor(xMin / step) * step; x <= xMax; x += step) {
                const px = toPx(x, 0)
                ctx.strokeStyle = (Math.abs(x) < 0.0001) ? Style.textMuted : Style.bgSection
                ctx.beginPath()
                ctx.moveTo(px.x, 0)
                ctx.lineTo(px.x, height)
                ctx.stroke()
                if (Math.abs(x) > 0.0001) {
                    ctx.fillText(x.toFixed(1), px.x + 2, height - 5)
                }
            }

            // Horizontal grid lines + y-axis labels.
            for (let y = Math.floor(yMin / step) * step; y <= yMax; y += step) {
                const py = toPx(0, y)
                ctx.strokeStyle = (Math.abs(y) < 0.0001) ? Style.textMuted : Style.bgSection
                ctx.beginPath()
                ctx.moveTo(0, py.y)
                ctx.lineTo(width, py.y)
                ctx.stroke()
                if (Math.abs(y) > 0.0001) {
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
            const multiPts = uiController.getMultiGraphPoints(600)
            const dotMode = uiController.drawMode === 1
            for (let f = 0; f < multiPts.length; f++) {
                const pts = multiPts[f]
                if (!pts || pts.length === 0) continue
                const colour = canvas.fnColors[f % canvas.fnColors.length]
                if (dotMode) {
                    ctx.fillStyle = colour
                    for (let i = 0; i < pts.length; i++) {
                        const p = toPx(pts[i].x, pts[i].y)
                        ctx.beginPath()
                        ctx.arc(p.x, p.y, 1.5, 0, 2 * Math.PI)
                        ctx.fill()
                    }
                } else {
                    ctx.beginPath()
                    ctx.strokeStyle = colour
                    ctx.lineWidth = 2
                    for (let i = 0; i < pts.length; i++) {
                        const p = toPx(pts[i].x, pts[i].y)
                        if (i === 0) ctx.moveTo(p.x, p.y)
                        else ctx.lineTo(p.x, p.y)
                    }
                    ctx.stroke()
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
                    ctx.strokeStyle = Style.bgShell
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

            // Trace marker + readout (drawn last so it sits on top).
            if (traceActive) {
                const tx = uiController.traceX
                const ty = uiController.traceY
                const activeIdx = uiController.activeFunctionIndex
                const traceColour = canvas.fnColors[activeIdx % canvas.fnColors.length]

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
                const readout = "Y" + (activeIdx + 1) +
                                "  X=" + uiController.formatScalar(tx) +
                                "  Y=" + (isFinite(ty)
                                          ? uiController.formatScalar(ty)
                                          : "—")
                ctx.font = "11px " + Style.monoFamily
                const textW = ctx.measureText(readout).width
                const padX = 6, padY = 4
                const rectW = textW + padX * 2
                const rectH = 18
                ctx.fillStyle = Style.bgShell
                ctx.globalAlpha = 0.85
                ctx.fillRect(0, height - rectH, rectW, rectH)
                ctx.globalAlpha = 1.0
                ctx.fillStyle = traceColour
                ctx.fillText(readout, padX, height - padY - 1)
            }
        }

        MouseArea {
            anchors.fill: parent
            property real lastX: 0
            property real lastY: 0
            onPressed: (mouse) => {
                lastX = mouse.x
                lastY = mouse.y
            }
            onPositionChanged: (mouse) => {
                if (pressed) {
                    uiController.pan(mouse.x - lastX, mouse.y - lastY, width, height)
                    lastX = mouse.x
                    lastY = mouse.y
                }
            }
            onWheel: (wheel) => {
                const factor = wheel.angleDelta.y > 0 ? 0.9 : 1.1
                uiController.zoom(factor, wheel.x, wheel.y, width, height)
            }
        }

        // Repaint on relevant controller signals.
        Connections {
            target: uiController
            function onViewportChanged() { canvas.requestPaint() }
            function onDisplayChanged() { canvas.requestPaint() }
            function onActiveFunctionIndexChanged() { canvas.requestPaint() }
            function onDrawModeChanged() { canvas.requestPaint() }
            function onGraphModeSettingChanged() { canvas.requestPaint() }
            function onStatPlotChanged() { canvas.requestPaint() }
            function onTraceChanged() { canvas.requestPaint() }
        }
    }
}
