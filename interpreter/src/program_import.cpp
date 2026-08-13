#include "program_import.hpp"

#include <cstdio>
#include <unordered_map>
#include <utility>

namespace tux_ti83 {
namespace {

inline uint16_t le16(const std::vector<uint8_t> &b, size_t off) {
  return static_cast<uint16_t>(b[off]) |
         (static_cast<uint16_t>(b[off + 1]) << 8);
}

// The .8xp container: 8-byte signature ("**TI83F*" for TI-83+, "**TI83**" for
// TI-83), 3 magic bytes, a 42-byte comment, then a 2-byte data-section length
// at 0x35, the data section at 0x37, and a trailing 2-byte checksum.
//
// The data section holds one variable entry:
//   +0  2  header size (0x0B or 0x0D)
//   +2  2  variable data length (= body length + 2)
//   +4  1  type ID (0x05 program / 0x06 protected)
//   +5  8  name
//   [+13 2  version + archive flag, only when header size is 0x0D]
//   then 2  body length N, then N body (token) bytes.
constexpr size_t kSigLen = 8;
constexpr size_t kDataLenOff = 0x35;
constexpr size_t kDataOff = 0x37;

// ── Two-byte-token prefixes ──────────────────────────────────────────────
bool isTwoBytePrefix(uint8_t b) {
  switch (b) {
    case 0x5C: case 0x5D: case 0x5E: case 0x60: case 0x62:
    case 0x63: case 0x7E: case 0xAA: case 0xBB: case 0xEF:
      return true;
    default:
      return false;
  }
}

// TI-83/84 Plus token → source-text tables. Decoded to the strings our own
// tokeniser accepts (ASCII operators, our glyphs for → ≤ ≥ ≠ √( π, the same
// command/function names), so an imported program re-tokenises + runs. Values
// cross-verified against the TI-Toolkit `8X.xml` token sheet. Tokens our
// engine doesn't implement still decode to their faithful TI text (readable;
// may need a manual edit to run). Built once, lazily.
const std::unordered_map<uint8_t, std::string> &oneByteTable() {
  static const std::unordered_map<uint8_t, std::string> t = [] {
    std::unordered_map<uint8_t, std::string> m = {
      {0x01,"▶DMS"},{0x02,"▶Dec"},{0x03,"▶Frac"},{0x04,"→"},
      {0x05,"Boxplot"},{0x06,"["},{0x07,"]"},{0x08,"{"},{0x09,"}"},
      {0x0A,"ʳ"},{0x0B,"°"},{0x0C,"^-1"},{0x0D,"^2"},{0x0E,"^T"},{0x0F,"^3"},
      {0x10,"("},{0x11,")"},{0x12,"round("},{0x13,"pxl-Test("},{0x14,"augment("},
      {0x15,"rowSwap("},{0x16,"row+("},{0x17,"*row("},{0x18,"*row+("},
      {0x19,"max("},{0x1A,"min("},{0x1B,"R▶Pr("},{0x1C,"R▶Pθ("},
      {0x1D,"P▶Rx("},{0x1E,"P▶Ry("},{0x1F,"median("},{0x20,"randM("},
      {0x21,"mean("},{0x22,"solve("},{0x23,"seq("},{0x24,"fnInt("},{0x25,"nDeriv("},
      {0x27,"fMin("},{0x28,"fMax("},{0x29," "},{0x2A,"\""},{0x2B,","},
      {0x2C,"i"},{0x2D,"!"},{0x2E,"CubicReg "},{0x2F,"QuartReg "},
      {0x3A,"."},{0x3B,"E"},{0x3C," xor "},{0x3E,":"},{0x3F,"\n"},{0x40," and "},
      {0x5B,"θ"},{0x5F,"prgm"},
      {0x64,"Radian"},{0x65,"Degree"},{0x66,"Normal"},{0x67,"Sci"},{0x68,"Eng"},
      {0x69,"Float"},{0x6A,"="},{0x6B,"<"},{0x6C,">"},{0x6D,"≤"},{0x6E,"≥"},
      {0x6F,"≠"},{0x70,"+"},{0x71,"-"},{0x72,"Ans"},{0x73,"Fix "},{0x74,"Horiz"},
      {0x75,"Full"},{0x76,"Func"},{0x77,"Param"},{0x78,"Polar"},{0x79,"Seq"},
      {0x82,"*"},{0x83,"/"},{0x84,"Trace"},{0x85,"ClrDraw"},{0x86,"ZStandard"},
      {0x87,"ZTrig"},{0x88,"ZBox"},{0x89,"Zoom In"},{0x8A,"Zoom Out"},{0x8B,"ZSquare"},
      {0x8C,"ZInteger"},{0x8D,"ZPrevious"},{0x8E,"ZDecimal"},{0x8F,"ZoomStat"},
      {0x90,"ZoomRcl"},{0x91,"PrintScreen"},{0x92,"ZoomSto"},{0x93,"Text("},
      {0x94," nPr "},{0x95," nCr "},{0x96,"FnOn "},{0x97,"FnOff "},{0x98,"StorePic "},
      {0x99,"RecallPic "},{0x9A,"StoreGDB "},{0x9B,"RecallGDB "},{0x9C,"Line("},
      {0x9D,"Vertical "},{0x9E,"Pt-On("},{0x9F,"Pt-Off("},{0xA0,"Pt-Change("},
      {0xA1,"Pxl-On("},{0xA2,"Pxl-Off("},{0xA3,"Pxl-Change("},{0xA4,"Shade("},
      {0xA5,"Circle("},{0xA6,"Horizontal "},{0xA7,"Tangent("},{0xA8,"DrawInv "},
      {0xA9,"DrawF "},{0xAB,"rand"},{0xAC,"π"},{0xAD,"getKey"},{0xAE,"'"},
      {0xAF,"?"},{0xB0,"-"},{0xB1,"int("},{0xB2,"abs("},{0xB3,"det("},
      {0xB4,"identity("},{0xB5,"dim("},{0xB6,"sum("},{0xB7,"prod("},{0xB8,"not("},
      {0xB9,"iPart("},{0xBA,"fPart("},{0xBC,"√("},{0xBD,"³√("},
      {0xBE,"ln("},{0xBF,"e^("},{0xC0,"log("},{0xC1,"10^("},{0xC2,"sin("},
      {0xC3,"sin^-1("},{0xC4,"cos("},{0xC5,"cos^-1("},{0xC6,"tan("},{0xC7,"tan^-1("},
      {0xC8,"sinh("},{0xC9,"sinh^-1("},{0xCA,"cosh("},{0xCB,"cosh^-1("},
      {0xCC,"tanh("},{0xCD,"tanh^-1("},{0xCE,"If "},{0xCF,"Then"},{0xD0,"Else"},
      {0xD1,"While "},{0xD2,"Repeat "},{0xD3,"For("},{0xD4,"End"},{0xD5,"Return"},
      {0xD6,"Lbl "},{0xD7,"Goto "},{0xD8,"Pause "},{0xD9,"Stop"},{0xDA,"IS>("},
      {0xDB,"DS<("},{0xDC,"Input "},{0xDD,"Prompt "},{0xDE,"Disp "},{0xDF,"DispGraph"},
      {0xE0,"Output("},{0xE1,"ClrHome"},{0xE2,"Fill("},{0xE3,"SortA("},{0xE4,"SortD("},
      {0xE5,"DispTable"},{0xE6,"Menu("},{0xE7,"Send("},{0xE8,"Get("},{0xE9,"PlotsOn "},
      {0xEA,"PlotsOff "},{0xEB,"L"},{0xEC,"Plot1("},{0xED,"Plot2("},{0xEE,"Plot3("},
      {0xF0,"^"},{0xF1,"ˣ√"},{0xF2,"1-Var Stats "},{0xF3,"2-Var Stats "},
      {0xF4,"LinReg(a+bx) "},{0xF5,"ExpReg "},{0xF6,"LnReg "},{0xF7,"PwrReg "},
      {0xF8,"Med-Med "},{0xF9,"QuadReg "},{0xFA,"ClrList "},{0xFB,"ClrTable"},
      {0xFC,"Histogram"},{0xFD,"xyLine"},{0xFE,"Scatter"},{0xFF,"LinReg(ax+b) "},
    };
    for (uint8_t d = 0; d <= 9; ++d)
      m[static_cast<uint8_t>(0x30 + d)] = std::string(1, char('0' + d));
    for (uint8_t l = 0; l < 26; ++l)
      m[static_cast<uint8_t>(0x41 + l)] = std::string(1, char('A' + l));
    return m;
  }();
  return t;
}
const std::unordered_map<uint16_t, std::string> &twoByteTable() {
  static const std::unordered_map<uint16_t, std::string> t = [] {
    std::unordered_map<uint16_t, std::string> m;
    auto put = [&](uint8_t p, uint8_t s, const char *str) {
      m[static_cast<uint16_t>(p) << 8 | s] = str;
    };
    // 0x5C — matrices [A]..[J]; 0x60 — Pic1..0; 0x61 — GDB1..0; 0xAA — Str1..0.
    for (uint8_t i = 0; i < 10; ++i) {
      char buf[8];
      std::snprintf(buf, sizeof buf, "[%c]", 'A' + i);          put(0x5C, i, buf);
      std::snprintf(buf, sizeof buf, "Pic%d", (i + 1) % 10);    put(0x60, i, buf);
      std::snprintf(buf, sizeof buf, "GDB%d", (i + 1) % 10);    put(0x61, i, buf);
      std::snprintf(buf, sizeof buf, "Str%d", (i + 1) % 10);    put(0xAA, i, buf);
    }
    // 0x5D — lists L1..L6.
    for (uint8_t i = 0; i < 6; ++i) {
      char buf[8]; std::snprintf(buf, sizeof buf, "L%d", i + 1); put(0x5D, i, buf);
    }
    // 0x5E — equation vars: Y1..Y0 (0x10..0x19), parametric, polar, seq.
    for (uint8_t i = 0; i < 10; ++i) {
      char buf[8]; std::snprintf(buf, sizeof buf, "Y%d", (i + 1) % 10);
      put(0x5E, 0x10 + i, buf);
    }
    for (uint8_t i = 0; i < 6; ++i) {
      char bx[8], by[8];
      std::snprintf(bx, sizeof bx, "X%dT", i + 1);
      std::snprintf(by, sizeof by, "Y%dT", i + 1);
      put(0x5E, 0x20 + 2 * i, bx);
      put(0x5E, 0x21 + 2 * i, by);
      char br[8]; std::snprintf(br, sizeof br, "r%d", i + 1); put(0x5E, 0x40 + i, br);
    }
    put(0x5E, 0x80, "u"); put(0x5E, 0x81, "v"); put(0x5E, 0x82, "w");
    // 0x63 — window / table variables (common).
    put(0x63,0x02,"Xscl"); put(0x63,0x03,"Yscl"); put(0x63,0x04,"u(nMin)");
    put(0x63,0x05,"v(nMin)"); put(0x63,0x0A,"Xmin"); put(0x63,0x0B,"Xmax");
    put(0x63,0x0C,"Ymin"); put(0x63,0x0D,"Ymax"); put(0x63,0x0E,"Tmin");
    put(0x63,0x0F,"Tmax"); put(0x63,0x1A,"TblStart"); put(0x63,0x1D,"nMax");
    put(0x63,0x1F,"nMin"); put(0x63,0x21,"∆Tbl"); put(0x63,0x36,"Xres");
    // 0x7E — graph-format settings.
    put(0x7E,0x00,"Sequential"); put(0x7E,0x01,"Simul"); put(0x7E,0x02,"PolarGC");
    put(0x7E,0x03,"RectGC"); put(0x7E,0x04,"CoordOn"); put(0x7E,0x05,"CoordOff");
    put(0x7E,0x08,"AxesOn"); put(0x7E,0x09,"AxesOff"); put(0x7E,0x0C,"LabelOn");
    put(0x7E,0x0D,"LabelOff");
    // 0xBB — extended page: functions, lowercase letters, common symbols.
    const std::pair<uint8_t, const char *> bb[] = {
      {0x0A,"randInt("},{0x0B,"randBin("},{0x0C,"sub("},{0x0D,"stdDev("},
      {0x0E,"variance("},{0x0F,"inString("},{0x10,"normalcdf("},{0x11,"invNorm("},
      {0x25,"conj("},{0x26,"real("},{0x27,"imag("},{0x28,"angle("},{0x29,"cumSum("},
      {0x2A,"expr("},{0x2B,"length("},{0x2C,"∆List("},{0x2D,"ref("},{0x2E,"rref("},
      {0x2F,"▶Rect"},{0x30,"▶Polar"},{0x31,"e"},{0x39,"Matr▶list("},
      {0x3A,"List▶matr("},{0x50,"ExprOn"},{0x51,"ExprOff"},{0x52,"ClrAllLists"},
      {0x54,"DelVar "},{0x55,"Equ▶String("},{0x56,"String▶Equ("},
      {0x57,"Clear Entries"},{0x65,"ZoomFit"},{0x68,"Archive "},{0x69,"UnArchive "},
      {0xA7,"π"},
    };
    for (const auto &e : bb) put(0xBB, e.first, e.second);
    // Lowercase letters: a..k at 0xB0..0xBA, then a GAP at 0xBB, l..z at 0xBC..0xCA.
    for (uint8_t i = 0; i < 11; ++i)  // a..k
      { char c = char('a' + i); put(0xBB, 0xB0 + i, std::string(1, c).c_str()); }
    for (uint8_t i = 0; i < 15; ++i)  // l..z
      { char c = char('l' + i); put(0xBB, 0xBC + i, std::string(1, c).c_str()); }
    return m;
  }();
  return t;
}

std::string decodeName(const std::vector<uint8_t> &ds) {
  std::string name;
  for (size_t i = 5; i < 13 && i < ds.size(); ++i) {
    const uint8_t c = ds[i];
    if (c == 0x00)
      break;
    if (c >= 0x41 && c <= 0x5A)
      name += static_cast<char>(c);            // A–Z
    else if (c >= 0x30 && c <= 0x39)
      name += static_cast<char>(c);            // 0–9
    else if (c == 0x5B)
      name += "theta";                          // θ — normalised away later
  }
  return name;
}

}  // namespace

Import8xpResult decode8xp(const std::vector<uint8_t> &bytes) {
  Import8xpResult r;

  if (bytes.size() < kDataOff + 2) {
    r.error = "Not a .8xp file (too small)";
    return r;
  }
  // Signature: accept "**TI83F*" (83+) and "**TI83**" (83). Both start "**TI83".
  const std::string sig(bytes.begin(), bytes.begin() + kSigLen);
  if (sig.rfind("**TI83", 0) != 0) {
    r.error = "Not a TI-83/84 file (bad signature)";
    return r;
  }

  const size_t dataLen = le16(bytes, kDataLenOff);
  if (dataLen == 0 || kDataOff + dataLen + 2 > bytes.size()) {
    r.error = "Corrupt file (data length out of range)";
    return r;
  }
  const std::vector<uint8_t> ds(bytes.begin() + kDataOff,
                                bytes.begin() + kDataOff + dataLen);
  if (ds.size() < 15) {
    r.error = "Corrupt file (variable header truncated)";
    return r;
  }
  const uint16_t headerSize = le16(ds, 0);
  if (headerSize != 0x0B && headerSize != 0x0D) {
    r.error = "Unsupported variable header";
    return r;
  }
  const uint8_t typeId = ds[4];
  if (typeId != 0x05 && typeId != 0x06) {
    r.error = "Not a program file (this is a different variable type)";
    return r;
  }
  r.name = decodeName(ds);

  const size_t bodyLenOff = 2 + headerSize;
  if (bodyLenOff + 2 > ds.size()) {
    r.error = "Corrupt file (missing body length)";
    return r;
  }
  const uint16_t bodyLen = le16(ds, bodyLenOff);
  const size_t bodyOff = bodyLenOff + 2;
  if (bodyOff + bodyLen > ds.size()) {
    r.error = "Corrupt file (body runs past end)";
    return r;
  }

  // ── Detokenise the body ──
  const auto &one = oneByteTable();
  const auto &two = twoByteTable();
  std::string out;
  out.reserve(bodyLen * 2);
  for (size_t i = 0; i < bodyLen;) {
    const uint8_t b = ds[bodyOff + i];
    if (isTwoBytePrefix(b) && i + 1 < bodyLen) {
      const uint16_t key =
          static_cast<uint16_t>(b) << 8 | ds[bodyOff + i + 1];
      auto it = two.find(key);
      if (it != two.end()) {
        out += it->second;
      } else {
        out += '?';
        ++r.unknownTokens;
      }
      i += 2;
      continue;
    }
    auto it = one.find(b);
    if (it != one.end()) {
      out += it->second;
    } else {
      out += '?';
      ++r.unknownTokens;
    }
    i += 1;
  }

  r.source = out;
  r.ok = true;
  return r;
}

}  // namespace tux_ti83
