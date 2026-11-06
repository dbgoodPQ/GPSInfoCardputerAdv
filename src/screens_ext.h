/*
 *  screens_ext.h
 *
 *  Native 320x240 layouts for the external Waveshare 2.0" LCD.
 *
 *  Every screen is laid out for 320x240 and drawn with fonts at their real
 *  pixel size (no setTextSize() scaling, no image zoom), so text and lines
 *  stay sharp:
 *
 *    F_SMALL  Font2             16 px proportional bitmap
 *    F_BODY   AsciiFont8x16      8x16 monospace bitmap (tables, 40 columns)
 *    F_MED    Font4             26 px proportional bitmap
 *    F_LARGE  FreeSansBold18pt   large values (coordinates)
 *    F_SEG48  Font7             48 px 7-segment digits (clock)
 *    F_SEG75  Font8             75 px digits (speed)
 *    F_TINY   Font0              6x8, only for dense labels next to plot dots
 *
 *  Included from Cardputer_GPS_Info.ino after all shared helpers and the
 *  original 240x135 screen functions.
 */
#pragma once

#define F_TINY   ((const lgfx::IFont*)&fonts::Font0)
#define F_SMALL  ((const lgfx::IFont*)&fonts::Font2)
#define F_BODY   ((const lgfx::IFont*)&fonts::AsciiFont8x16)
#define F_MED    ((const lgfx::IFont*)&fonts::Font4)
#define F_LARGE  ((const lgfx::IFont*)&fonts::FreeSansBold18pt7b)
#define F_SEG48  ((const lgfx::IFont*)&fonts::Font7)
#define F_SEG75  ((const lgfx::IFont*)&fonts::Font8)

// ------------------------------------------------------------------
//  Text helpers
// ------------------------------------------------------------------

/*    Draw a string with an explicit font and alignment. Returns its width.
*/
static int xText(const char* s, int x, int y, uint16_t col,
                 const lgfx::IFont* f, textdatum_t datum = TL_DATUM) {
  frameBuf.setFont(f);
  frameBuf.setTextSize(1);
  frameBuf.setTextDatum(datum);
  frameBuf.setTextColor(col);
  int w = frameBuf.drawString(s, x, y);
  frameBuf.setTextDatum(TL_DATUM);
  return w;
}

static int xTextW(const char* s, const lgfx::IFont* f) {
  frameBuf.setFont(f);
  frameBuf.setTextSize(1);
  return frameBuf.textWidth(s);
}

static int xFontH(const lgfx::IFont* f) {
  frameBuf.setFont(f);
  frameBuf.setTextSize(1);
  return frameBuf.fontHeight();
}

/*    "pre" + degree ring + "post" as one run of text. The bitmap fonts have
*     no degree glyph, so the ring is drawn. align: 0 = left, 1 = centre.
*/
static void xTextDeg(const char* pre, const char* post, int x, int y,
                     uint16_t col, const lgfx::IFont* f, int align = 0) {
  int fh = xFontH(f);
  int r = (fh >= 24) ? 3 : 2;
  int ringW = r * 2 + 3;
  int w1 = xTextW(pre, f);
  int w2 = xTextW(post, f);
  int total = w1 + ringW + w2;
  int x0 = (align == 1) ? x - total / 2 : x;
  xText(pre, x0, y, col, f);
  int ry = y + r + (fh >= 24 ? 3 : 2);
  frameBuf.drawCircle(x0 + w1 + r + 1, ry, r, col);
  if (r >= 3) frameBuf.drawCircle(x0 + w1 + r + 1, ry, r - 1, col);
  xText(post, x0 + w1 + ringW, y, col, f);
}

static void xCentered(const char* s, int y, uint16_t col, const lgfx::IFont* f) {
  xText(s, screenW / 2, y, col, f, TC_DATUM);
}

static const char* fixQualityName(bool longName) {
  if (ggaFixQuality == 1) return "GPS";
  if (ggaFixQuality == 2) return "DGPS";
  if (ggaFixQuality == 4) return "RTK";
  if (ggaFixQuality == 5) return longName ? "Float RTK" : "FRTK";
  return "None";
}

// ------------------------------------------------------------------
//  Sky plot (parametric, 320x240 style)
// ------------------------------------------------------------------
void drawSkyPlotExt(int x, int y, int w, int h, bool showIds, bool showSys) {
  int side = (w < h) ? w : h;
  int r = side / 2 - 1;
  int cx = x + w / 2;
  int cy = y + h / 2;
  frameBuf.drawRect(x - 1, y - 1, w + 2, h + 2, TFT_DARKGREY);
  frameBuf.fillRect(x, y, w, h, TFT_BLACK);
  frameBuf.drawCircle(cx, cy, r, TFT_WHITE);
  frameBuf.drawCircle(cx, cy, r * 2 / 3, TFT_DARKGREY);
  frameBuf.drawCircle(cx, cy, r / 3, TFT_DARKGREY);
  frameBuf.drawLine(cx - r, cy, cx + r, cy, TFT_DARKGREY);
  frameBuf.drawLine(cx, cy - r, cx, cy + r, TFT_DARKGREY);

  const lgfx::IFont* cf = (r >= 70) ? F_SMALL : F_TINY;
  int pad = (r >= 70) ? 3 : 2;
  xText("N", cx, cy - r + pad, TFT_LIGHTGREY, cf, TC_DATUM);
  xText("S", cx, cy + r - pad, TFT_LIGHTGREY, cf, BC_DATUM);
  xText("E", cx + r - pad, cy, TFT_LIGHTGREY, cf, MR_DATUM);
  xText("W", cx - r + pad, cy, TFT_LIGHTGREY, cf, ML_DATUM);

  int dotR = (r >= 90) ? 5 : (r >= 60) ? 4 : 3;
  const lgfx::IFont* lf = (r >= 90) ? F_SMALL : F_TINY;
  for (auto &sat : satellites) {
    float elev = constrain(sat.elevation, 0.0, 90.0);
    float az   = fmod(sat.azimuth + 360.0, 360.0);
    float rad  = (90.0 - elev) / 90.0 * r;
    float radAz = radians(az);
    int sx = cx + (int)(rad * sin(radAz));
    int sy = cy - (int)(rad * cos(radAz));
    uint16_t color = TFT_RED;
    if (sat.used) color = TFT_GREEN;
    else if (sat.visible) color = TFT_YELLOW;
    frameBuf.fillCircle(sx, sy, dotR, color);
    char lbl[12] = "";
    if (showIds) snprintf(lbl, sizeof(lbl), "%d", sat.id);
    if (showSys) {
      const char* sys = "?";
      if (sat.system == "GPS") sys = "Gp";
      else if (sat.system == "GLONASS") sys = "Gl";
      else if (sat.system == "Galileo") sys = "Ga";
      else if (sat.system == "BeiDou") sys = "Bd";
      else if (sat.system == "QZSS") sys = "Qz";
      size_t n = strlen(lbl);
      snprintf(lbl + n, sizeof(lbl) - n, "%s%s", n ? " " : "", sys);
    }
    if (lbl[0]) {
      int lx = sx + dotR + 2;
      int ly = sy - dotR - 1;
      xText(lbl, lx + 1, ly + 1, TFT_BLACK, lf, BL_DATUM);  // shadow
      xText(lbl, lx, ly, color, lf, BL_DATUM);
    }
  }
}

// ------------------------------------------------------------------
//  Screen 0: Main (data table + sky plot)
// ------------------------------------------------------------------
void drawMainExt() {
  const int rowH = 29, y0 = 4;
  const int c1x = 2, c1w = 130;      // fits "Lng -123.456789" (15 x 8 px)
  const int c2x = c1x + c1w - 1, c2w = 96;   // fits "Gp/Gl 12/12"
  const int skyX = c2x + c2w + 3, skyW = 318 - skyX;

  char c1[8][24];
  if (gps.location.isValid()) snprintf(c1[0], 24, "Lat %.6f", gps.location.lat()); else snprintf(c1[0], 24, "Lat NoFix");
  if (gps.location.isValid()) snprintf(c1[1], 24, "Lng %.6f", gps.location.lng()); else snprintf(c1[1], 24, "Lng NoFix");
  if (gps.altitude.isValid()) snprintf(c1[2], 24, "Alt %.1fm", gps.altitude.meters()); else snprintf(c1[2], 24, "Alt NoData");
  snprintf(c1[3], 24, "Spd %.1f", gps.speed.kmph());
  snprintf(c1[4], 24, "Crs %.1f", gps.course.deg());
  if (gps.date.isValid()) snprintf(c1[5], 24, "Date %02d/%02d/%02d", gps.date.day(), gps.date.month(), gps.date.year() % 100);
  else snprintf(c1[5], 24, "Date NoData");
  if (gps.time.isValid()) snprintf(c1[6], 24, "Time %02d:%02d:%02d", gps.time.hour(), gps.time.minute(), gps.time.second());
  else snprintf(c1[6], 24, "Time NoData");
  snprintf(c1[7], 24, "Fix %s%s", fixQualityName(false),
           gsaFixMode == 3 ? " 3D" : gsaFixMode == 2 ? " 2D" : "");

  SatCounts sc;
  countSatellites(sc);
  qzssVisible = sc.qzssVis;
  char c2[8][16];
  snprintf(c2[0], 16, "Vis %d/%d", sc.totalVisible, sc.total);
  snprintf(c2[1], 16, "Usd %d", gps.satellites.isValid() ? (int)gps.satellites.value() : sc.totalUsed);
  snprintf(c2[2], 16, "PD %.1f", pdop);
  snprintf(c2[3], 16, "HD %.1f", gps.hdop.hdop());
  snprintf(c2[4], 16, "VD %.1f", vdop);
  snprintf(c2[5], 16, "Gp/Gl %d/%d", sc.gpsVis, sc.glonassVis);
  snprintf(c2[6], 16, "Ga/Bd %d/%d", sc.galileoVis, sc.beidouVis);
  snprintf(c2[7], 16, "Qz %d", sc.qzssVis);

  for (int i = 0; i < 8; i++) {
    int y = y0 + i * rowH;
    frameBuf.drawRect(c1x, y, c1w, rowH + 1, TFT_DARKGREY);
    frameBuf.drawRect(c2x, y, c2w, rowH + 1, TFT_DARKGREY);
    // Label in grey, value in white.
    char lab[8]; const char* sp = strchr(c1[i], ' ');
    int n = sp ? (int)(sp - c1[i]) : 0;
    snprintf(lab, sizeof(lab), "%.*s", n, c1[i]);
    int lw = xText(lab, c1x + 4, y + 7, TFT_LIGHTGREY, F_BODY);
    xText(sp ? sp : c1[i], c1x + 4 + lw, y + 7, TFT_WHITE, F_BODY);
    sp = strchr(c2[i], ' ');
    n = sp ? (int)(sp - c2[i]) : 0;
    snprintf(lab, sizeof(lab), "%.*s", n, c2[i]);
    lw = xText(lab, c2x + 4, y + 7, TFT_LIGHTGREY, F_BODY);
    xText(sp ? sp : c2[i], c2x + 4 + lw, y + 7, TFT_WHITE, F_BODY);
  }
  drawSkyPlotExt(skyX, y0 + 1, skyW, rowH * 8 - 1, !hidePlotId, !hidePlotSystem);
}

// ------------------------------------------------------------------
//  Screen 1: Sky view
// ------------------------------------------------------------------
void drawSkyViewExt() {
  const int size = 232;
  drawSkyPlotExt(4, 4, size, size, !hidePlotId, !hidePlotSystem);
  SatCounts sc;
  countSatellites(sc);
  int lost = sc.total - sc.totalVisible;
  int x = 246, y = 14;
  struct { const char* name; int n; uint16_t col; } rows[] = {
    {"Used", sc.totalUsed, TFT_GREEN},
    {"Vis",  sc.totalVisible, TFT_YELLOW},
    {"Lost", lost < 0 ? 0 : lost, TFT_RED},
  };
  char buf[12];
  for (auto &r : rows) {
    frameBuf.fillCircle(x + 5, y + 8, 5, r.col);
    xText(r.name, x + 16, y, TFT_WHITE, F_BODY);
    snprintf(buf, sizeof(buf), "%d", r.n);
    xText(buf, x + 16, y + 20, r.col, F_MED);
    y += 58;
  }
  xText("[p] ID", x, 196, hidePlotId ? TFT_DARKGREY : TFT_GREEN, F_SMALL);
  xText("[o] Sys", x, 216, hidePlotSystem ? TFT_DARKGREY : TFT_GREEN, F_SMALL);
}

// ------------------------------------------------------------------
//  Screen 2: Signal bars
// ------------------------------------------------------------------
void drawSignalBarsExt() {
  const int chartX = 34, chartY = 30, chartW = 280, chartH = 150;
  const int baseY = chartY + chartH;   // ID labels: 1-2 rows of 16 px below

  std::vector<SatData*> visSats;
  for (auto &sat : satellites) if (sat.visible) visSats.push_back(&sat);
  if (visSats.empty()) {
    xCentered("No satellites visible", 112, TFT_DARKGREY, F_MED);
    return;
  }
  std::sort(visSats.begin(), visSats.end(), [](SatData* a, SatData* b) {
    if (a->system != b->system) return a->system < b->system;
    return a->id < b->id;
  });

  int numSats = visSats.size();
  int maxBars = (numSats > 32) ? 32 : numSats;
  int gap = 2;
  int barW = (chartW - maxBars * gap) / maxBars;
  if (barW < 4) barW = 4;
  if (barW > 20) barW = 20;
  int totalBarW = maxBars * (barW + gap);
  int startX = chartX + (chartW - totalBarW) / 2;

  char buf[32];
  snprintf(buf, sizeof(buf), "SNR (dB)   %d sats", numSats);
  xText(buf, chartX, 6, TFT_WHITE, F_BODY);

  for (int v = 0; v <= 50; v += 25) {
    int ly = baseY - (v * chartH / 50);
    frameBuf.drawFastHLine(chartX, ly, chartW, TFT_DARKGREY);
    snprintf(buf, sizeof(buf), "%d", v);
    xText(buf, chartX - 5, ly, TFT_DARKGREY, F_BODY, MR_DATUM);
  }

  const lgfx::IFont* valF = (barW >= 16) ? F_SMALL : (barW >= 12) ? F_TINY : nullptr;
  // IDs in 8x16 text: one row if a 2-digit ID fits the bar pitch,
  // otherwise alternate between two rows so neighbours never overlap.
  const int pitch = barW + gap;
  const bool stagger = pitch < 18;
  const lgfx::IFont* idF = (2 * pitch >= 17) ? F_BODY : F_TINY;
  for (int i = 0; i < maxBars; i++) {
    SatData* sat = visSats[i];
    int snr = constrain(sat->snr, 0, 50);
    int barH = (snr * chartH) / 50;
    int bx = startX + i * (barW + gap);
    uint16_t color = systemColor(sat->system);
    if (sat->used && barH > 0)
      frameBuf.drawRect(bx - 1, baseY - barH - 1, barW + 2, barH + 2, TFT_WHITE);
    if (barH > 0)
      frameBuf.fillRect(bx, baseY - barH, barW, barH, color);
    if (valF && snr > 0) {
      snprintf(buf, sizeof(buf), "%d", snr);
      xText(buf, bx + barW / 2, baseY - barH - 3, TFT_WHITE, valF, BC_DATUM);
    }
    snprintf(buf, sizeof(buf), "%d", sat->id);
    int room = stagger ? 2 * pitch : pitch;
    if (strlen(buf) > 2 && room < 25) buf[2] = '\0';   // as upstream: 2 digits when narrow
    int rowY = baseY + 4 + ((stagger && (i & 1)) ? 17 : 0);
    xText(buf, bx + barW / 2, rowY, color, idF, TC_DATUM);
  }

  int legX = 6, legY = 222;
  struct { const char* label; uint16_t color; } legend[] = {
    {"GP", TFT_GREEN}, {"GL", TFT_RED}, {"GA", TFT_CYAN},
    {"BD", TFT_YELLOW}, {"QZ", TFT_MAGENTA}
  };
  for (auto &l : legend) {
    frameBuf.fillRect(legX, legY + 3, 10, 10, l.color);
    legX += 14 + xText(l.label, legX + 14, legY, l.color, F_BODY) + 10;
  }
  frameBuf.drawRect(legX + 2, legY + 2, 10, 12, TFT_WHITE);
  xText("= Used", legX + 16, legY, TFT_WHITE, F_BODY);
}

// ------------------------------------------------------------------
//  Screen 3: Fix summary
// ------------------------------------------------------------------
void drawFixSummaryExt() {
  const int x = 10, lh = 25;
  int y = 8;
  char buf[48];

  snprintf(buf, sizeof(buf), "Fix Quality: %s", fixQualityName(true));
  xText(buf, x, y, TFT_GREEN, F_BODY); y += lh;
  snprintf(buf, sizeof(buf), "Fix Mode:    %s",
           gsaFixMode == 3 ? "3D" : gsaFixMode == 2 ? "2D" : "No Fix");
  xText(buf, x, y, TFT_WHITE, F_BODY); y += lh;
  snprintf(buf, sizeof(buf), "PDOP: %4.1f  (%s)", pdop, dopQuality(pdop));
  xText(buf, x, y, TFT_WHITE, F_BODY); y += lh;
  float hd = gps.hdop.hdop();
  snprintf(buf, sizeof(buf), "HDOP: %4.1f  (%s)", hd, dopQuality(hd));
  xText(buf, x, y, TFT_WHITE, F_BODY); y += lh;
  snprintf(buf, sizeof(buf), "VDOP: %4.1f  (%s)", vdop, dopQuality(vdop));
  xText(buf, x, y, TFT_WHITE, F_BODY); y += lh;

  SatCounts sc;
  countSatellites(sc);
  snprintf(buf, sizeof(buf), "Used: %d  Visible: %d  Total: %d", sc.totalUsed, sc.totalVisible, sc.total);
  xText(buf, x, y, TFT_LIGHTGREY, F_BODY); y += lh;
  snprintf(buf, sizeof(buf), "GP:%d GL:%d GA:%d BD:%d QZ:%d",
           sc.gpsVis, sc.glonassVis, sc.galileoVis, sc.beidouVis, sc.qzssVis);
  xText(buf, x, y, TFT_LIGHTGREY, F_BODY); y += lh;
  if (geoidValid) snprintf(buf, sizeof(buf), "Geoid Height: %.1f m", geoidHeight);
  else            snprintf(buf, sizeof(buf), "Geoid Height: N/A");
  xText(buf, x, y, TFT_LIGHTGREY, F_BODY); y += lh;
  if (imuAvailable) {
    snprintf(buf, sizeof(buf), "IMU P:%+.0f R:%+.0f G:%.1f T:%.0fC", imuPitch, imuRoll, imuGforce, imuTemp);
    xText(buf, x, y, TFT_LIGHTGREY, F_BODY);
  }
}

// ------------------------------------------------------------------
//  Screen 4: Dashboard
// ------------------------------------------------------------------
void drawDashboardExt() {
  char buf[32];
  xCentered("km/h", 6, TFT_DARKGREY, F_BODY);
  snprintf(buf, sizeof(buf), "%.1f", gps.speed.kmph());
  xCentered(buf, 26, TFT_GREEN, F_SEG75);

  if (gps.course.isValid()) {
    char pre[12], post[8];
    snprintf(pre, sizeof(pre), "HDG %.0f", gps.course.deg());
    snprintf(post, sizeof(post), " %s", cardinalFromHeading(gps.course.deg()));
    xTextDeg(pre, post, screenW / 2, 116, TFT_WHITE, F_MED, 1);
  } else {
    xCentered("HDG ---", 116, TFT_WHITE, F_MED);
  }

  if (gps.altitude.isValid()) snprintf(buf, sizeof(buf), "ALT %.0f m", gps.altitude.meters());
  else                        snprintf(buf, sizeof(buf), "ALT ---");
  xCentered(buf, 154, TFT_CYAN, F_MED);

  if (imuAvailable) {
    snprintf(buf, sizeof(buf), "P:%+.0f  R:%+.0f  G:%.1f  T:%.0fC", imuPitch, imuRoll, imuGforce, imuTemp);
    xCentered(buf, 206, TFT_LIGHTGREY, F_BODY);
  }
}

// ------------------------------------------------------------------
//  Screen 5: Coordinates
// ------------------------------------------------------------------
static void coordBlockExt(const char* label, float v, bool isLat, int y) {
  char buf[32], pre[8], post[24];
  xText(label, 10, y, TFT_DARKGREY, F_BODY);
  snprintf(buf, sizeof(buf), "%.6f", v);
  xText(buf, 10, y + 20, TFT_GREEN, F_LARGE);
  float a = fabs(v);
  int d = (int)a;
  float rem = (a - d) * 60.0;
  int m = (int)rem;
  float s = (rem - m) * 60.0;
  snprintf(pre, sizeof(pre), "%d", d);
  snprintf(post, sizeof(post), " %d' %.1f\" %s", m, s,
           isLat ? (v >= 0 ? "N" : "S") : (v >= 0 ? "E" : "W"));
  xTextDeg(pre, post, 10, y + 64, TFT_WHITE, F_MED, 0);
}

void drawCoordinatesExt() {
  if (!gps.location.isValid()) {
    xCentered("No Fix", 106, TFT_DARKGREY, F_MED);
    return;
  }
  coordBlockExt("LAT", gps.location.lat(), true, 8);
  frameBuf.drawFastHLine(10, 118, 300, 0x2104);
  coordBlockExt("LON", gps.location.lng(), false, 126);
}

// ------------------------------------------------------------------
//  Screen 6: Breadcrumb track
// ------------------------------------------------------------------
void drawBreadcrumbExt() {
  const int mapX = 4, mapY = 4, mapW = 312, mapH = 232;
  if (trackCount < 1) {
    xCentered("No track data yet", 112, TFT_DARKGREY, F_MED);
    return;
  }
  int latestIdx = (trackHead - 1 + TRACK_MAX) % TRACK_MAX;
  float cLat = trackBuf[latestIdx].lat;
  float cLon = trackBuf[latestIdx].lon;
  float cosLat = cos(radians(cLat));
  float px[TRACK_MAX], py[TRACK_MAX];
  float maxDist = 1.0;
  for (int i = 0; i < trackCount; i++) {
    int idx = (trackHead - trackCount + i + TRACK_MAX) % TRACK_MAX;
    px[i] = (trackBuf[idx].lon - cLon) * cosLat * 111320.0;
    py[i] = (trackBuf[idx].lat - cLat) * 111320.0;
    float d = max(fabs(px[i]), fabs(py[i]));
    if (d > maxDist) maxDist = d;
  }
  float halfMap = min((float)mapW, (float)mapH) / 2.0;
  float scale = halfMap / (maxDist * 1.2);
  int mcx = mapX + mapW / 2;
  int mcy = mapY + mapH / 2;

  frameBuf.drawRect(mapX, mapY, mapW, mapH, TFT_DARKGREY);
  for (int i = 1; i < trackCount; i++) {
    int sx1 = constrain(mcx + (int)(px[i-1] * scale), mapX + 1, mapX + mapW - 2);
    int sy1 = constrain(mcy - (int)(py[i-1] * scale), mapY + 1, mapY + mapH - 2);
    int sx2 = constrain(mcx + (int)(px[i] * scale),   mapX + 1, mapX + mapW - 2);
    int sy2 = constrain(mcy - (int)(py[i] * scale),   mapY + 1, mapY + mapH - 2);
    uint16_t col = (i > trackCount / 2) ? TFT_GREEN : TFT_DARKGREY;
    frameBuf.drawLine(sx1, sy1, sx2, sy2, col);
  }
  frameBuf.fillCircle(mcx, mcy, 4, TFT_RED);

  // North arrow.
  int ax = mapX + mapW - 18, at = mapY + 8, ab = mapY + 26;
  frameBuf.drawLine(ax, ab, ax, at, TFT_WHITE);
  frameBuf.fillTriangle(ax, at - 2, ax - 4, at + 6, ax + 4, at + 6, TFT_WHITE);
  xText("N", ax, ab + 2, TFT_WHITE, F_BODY, TC_DATUM);

  // Span of the view (half-width in metres).
  char buf[24];
  float spanM = halfMap / scale;
  if (spanM < 1000) snprintf(buf, sizeof(buf), "+/- %.0f m", spanM);
  else              snprintf(buf, sizeof(buf), "+/- %.1f km", spanM / 1000.0);
  xText(buf, mapX + 6, mapY + mapH - 20, TFT_DARKGREY, F_SMALL);
}

// ------------------------------------------------------------------
//  Screens 7/8: Altitude profile and speed graph
// ------------------------------------------------------------------
static void lineGraphExt(const float* vals, int n, float minV, float maxV,
                         uint16_t col, const char* title, const char* nowStr) {
  const int gx = 50, gy = 32, gw = 262, gh = 184;
  float range = maxV - minV;
  frameBuf.drawFastVLine(gx, gy, gh + 1, TFT_DARKGREY);
  frameBuf.drawFastHLine(gx, gy + gh, gw + 1, TFT_DARKGREY);
  // Mid grid line.
  for (int x = gx + 2; x < gx + gw; x += 4)
    frameBuf.drawPixel(x, gy + gh / 2, TFT_DARKGREY);

  char buf[16];
  snprintf(buf, sizeof(buf), "%.0f", maxV);
  xText(buf, gx - 5, gy, TFT_DARKGREY, F_BODY, TR_DATUM);
  snprintf(buf, sizeof(buf), "%.0f", (minV + maxV) / 2);
  xText(buf, gx - 5, gy + gh / 2, TFT_DARKGREY, F_BODY, MR_DATUM);
  snprintf(buf, sizeof(buf), "%.0f", minV);
  xText(buf, gx - 5, gy + gh, TFT_DARKGREY, F_BODY, BR_DATUM);

  for (int i = 1; i < n; i++) {
    int x1 = gx + (i - 1) * gw / (n - 1);
    int y1 = gy + gh - (int)((vals[i-1] - minV) / range * gh);
    int x2 = gx + i * gw / (n - 1);
    int y2 = gy + gh - (int)((vals[i] - minV) / range * gh);
    frameBuf.drawLine(x1, y1, x2, y2, col);
    frameBuf.drawLine(x1, y1 - 1, x2, y2 - 1, col);   // 2 px line
  }
  xText(title, gx + 4, 8, TFT_WHITE, F_BODY);
  xText(nowStr, 316, 8, col, F_BODY, TR_DATUM);

  // Time span of the buffer (one point every 2 s).
  int spanS = (n - 1) * 2;
  snprintf(buf, sizeof(buf), "-%dm%02ds", spanS / 60, spanS % 60);
  xText(buf, gx, gy + gh + 4, TFT_DARKGREY, F_SMALL);
  xText("now", gx + gw, gy + gh + 4, TFT_DARKGREY, F_SMALL, TR_DATUM);
}

void drawAltProfileExt() {
  if (trackCount < 2) {
    xCentered("Collecting altitude data...", 112, TFT_DARKGREY, F_MED);
    return;
  }
  float vals[TRACK_MAX];
  float minV = 99999, maxV = -99999;
  for (int i = 0; i < trackCount; i++) {
    int idx = (trackHead - trackCount + i + TRACK_MAX) % TRACK_MAX;
    vals[i] = trackBuf[idx].altM;
    if (vals[i] < minV) minV = vals[i];
    if (vals[i] > maxV) maxV = vals[i];
  }
  float curV = vals[trackCount - 1];
  if (maxV - minV < 1.0) { minV -= 0.5; maxV += 0.5; }
  char now[24];
  snprintf(now, sizeof(now), "Now: %.0f m", curV);
  lineGraphExt(vals, trackCount, minV, maxV, TFT_CYAN, "Altitude (m)", now);
}

void drawSpeedGraphExt() {
  if (trackCount < 2) {
    xCentered("Collecting speed data...", 112, TFT_DARKGREY, F_MED);
    return;
  }
  float vals[TRACK_MAX];
  float minV = 99999, maxV = -99999, sumV = 0;
  for (int i = 0; i < trackCount; i++) {
    int idx = (trackHead - trackCount + i + TRACK_MAX) % TRACK_MAX;
    vals[i] = trackBuf[idx].speedKmph;
    if (vals[i] < minV) minV = vals[i];
    if (vals[i] > maxV) maxV = vals[i];
    sumV += vals[i];
  }
  float curV = vals[trackCount - 1];
  float avgV = sumV / trackCount;
  if (minV > 0) minV = 0;
  if (maxV - minV < 1.0) maxV = minV + 1.0;
  char title[32], now[20];
  snprintf(title, sizeof(title), "Speed (km/h)  avg %.1f", avgV);
  snprintf(now, sizeof(now), "Now: %.1f", curV);
  lineGraphExt(vals, trackCount, minV, maxV, TFT_YELLOW, title, now);
}

// ------------------------------------------------------------------
//  Screen 9: Trip statistics
// ------------------------------------------------------------------
void drawTripStatsExt() {
  if (!trip.hasPrev) {
    xCentered("No trip data yet", 112, TFT_DARKGREY, F_MED);
    return;
  }
  const int x = 10, lh = 25;
  int y = 8;
  char buf[48], elBuf[12], mvBuf[12];
  uint32_t elapsed = millis() - trip.startMillis;
  float avgSpd = (trip.movingMillis > 1000) ? (trip.totalDistKm / (trip.movingMillis / 3600000.0)) : 0;
  formatDuration(elapsed, elBuf, sizeof(elBuf));
  formatDuration(trip.movingMillis, mvBuf, sizeof(mvBuf));

  snprintf(buf, sizeof(buf), "Distance:  %.2f km", trip.totalDistKm);   xText(buf, x, y, TFT_GREEN, F_BODY); y += lh;
  snprintf(buf, sizeof(buf), "Elapsed:   %s", elBuf);                    xText(buf, x, y, TFT_WHITE, F_BODY); y += lh;
  snprintf(buf, sizeof(buf), "Moving:    %s", mvBuf);                    xText(buf, x, y, TFT_WHITE, F_BODY); y += lh;
  snprintf(buf, sizeof(buf), "Avg Speed: %.1f km/h", avgSpd);            xText(buf, x, y, TFT_WHITE, F_BODY); y += lh;
  snprintf(buf, sizeof(buf), "Max Speed: %.1f km/h", trip.maxSpeedKmph); xText(buf, x, y, TFT_WHITE, F_BODY); y += lh;
  snprintf(buf, sizeof(buf), "Alt Gain:  +%.0f m", trip.totalAscentM);   xText(buf, x, y, TFT_CYAN,  F_BODY); y += lh;
  snprintf(buf, sizeof(buf), "Alt Loss:  -%.0f m", trip.totalDescentM);  xText(buf, x, y, TFT_CYAN,  F_BODY); y += lh;
  snprintf(buf, sizeof(buf), "Alt Range: %.0f - %.0f m", trip.minAltM, trip.maxAltM);
  xText(buf, x, y, TFT_CYAN, F_BODY); y += lh;
  if (imuAvailable) {
    snprintf(buf, sizeof(buf), "Max G:     %.2f", tripMaxG);
    xText(buf, x, y, TFT_YELLOW, F_BODY);
  }
}

// ------------------------------------------------------------------
//  Screen 10: Constellation breakdown
// ------------------------------------------------------------------
void drawConstellationExt() {
  SatCounts sc;
  countSatellites(sc);
  const int nameX = 32, visX = 224, usedX = 304, lh = 30;
  int y = 10;
  xText("System",  nameX, y, TFT_DARKGREY, F_BODY);
  xText("Visible", visX,  y, TFT_DARKGREY, F_BODY, TR_DATUM);
  xText("Used",    usedX, y, TFT_DARKGREY, F_BODY, TR_DATUM);
  frameBuf.drawFastHLine(8, 32, 304, TFT_DARKGREY);

  struct { const char* name; int vis, used; uint16_t col; } rows[] = {
    {"GPS",     sc.gpsVis,     sc.gpsUsed,     TFT_GREEN},
    {"GLONASS", sc.glonassVis, sc.glonassUsed, TFT_RED},
    {"Galileo", sc.galileoVis, sc.galileoUsed, TFT_CYAN},
    {"BeiDou",  sc.beidouVis,  sc.beidouUsed,  TFT_YELLOW},
    {"QZSS",    sc.qzssVis,    sc.qzssUsed,    TFT_MAGENTA},
  };
  char buf[8];
  y = 40;
  for (auto &r : rows) {
    frameBuf.fillCircle(16, y + 8, 5, r.col);
    xText(r.name, nameX, y, TFT_WHITE, F_BODY);
    snprintf(buf, sizeof(buf), "%d", r.vis);  xText(buf, visX,  y, TFT_WHITE, F_BODY, TR_DATUM);
    snprintf(buf, sizeof(buf), "%d", r.used); xText(buf, usedX, y, TFT_WHITE, F_BODY, TR_DATUM);
    y += lh;
  }
  frameBuf.drawFastHLine(8, y - 6, 304, TFT_DARKGREY);
  xText("Total", nameX, y + 2, TFT_GREEN, F_BODY);
  snprintf(buf, sizeof(buf), "%d", sc.totalVisible); xText(buf, visX,  y + 2, TFT_GREEN, F_BODY, TR_DATUM);
  snprintf(buf, sizeof(buf), "%d", sc.totalUsed);    xText(buf, usedX, y + 2, TFT_GREEN, F_BODY, TR_DATUM);
}

// ------------------------------------------------------------------
//  Screen 11: NMEA monitor (40 columns x 15 lines of 8x16 text)
// ------------------------------------------------------------------
void drawNmeaMonitorExt() {
  const int lines = screenH / 16;      // 15
  const int cols  = screenW / 8;       // 40
  int maxLines = (nmeaBufCount < lines) ? nmeaBufCount : lines;
  if (maxLines == 0) {
    xCentered("Waiting for NMEA data...", 112, TFT_DARKGREY, F_MED);
    return;
  }
  char row[48];
  for (int i = 0; i < maxLines; i++) {
    int idx = (nmeaBufHead - maxLines + i + NMEA_BUF_LINES) % NMEA_BUF_LINES;
    strncpy(row, nmeaBuf[idx], cols);
    row[cols] = '\0';
    xText(row, 0, i * 16, TFT_GREEN, F_BODY);
  }
}

// ------------------------------------------------------------------
//  Screen 12: GPS clock
// ------------------------------------------------------------------
void drawGpsClockExt() {
  char buf[32];
  if (!gps.time.isValid()) {
    xCentered("No Time Data", 106, TFT_DARKGREY, F_MED);
    return;
  }
  int utcH = gps.time.hour(), utcM = gps.time.minute(), utcS = gps.time.second();
  bool hasLoc = gps.location.isValid();
  int stdOff = 0, dstAdj = 0;
  if (hasLoc) {
    stdOff = (int)roundf(gps.location.lng() / 15.0f);
    if (gps.date.isValid())
      dstAdj = getDST(gps.date.year(), gps.date.month(), gps.date.day(),
                      utcH, gps.location.lat(), gps.location.lng());
  }
  int tzOff = stdOff + dstAdj;
  int localH = utcH, localMin = utcM, localSec = utcS, localY = 0, localMon = 0, localD = 0;
  bool localDateValid = false;
  if (gps.date.isValid()) {
    struct tm utcTm = {};
    utcTm.tm_year = gps.date.year() - 1900;
    utcTm.tm_mon  = gps.date.month() - 1;
    utcTm.tm_mday = gps.date.day();
    utcTm.tm_hour = utcH; utcTm.tm_min = utcM; utcTm.tm_sec = utcS;
    time_t epoch = mktime(&utcTm) + (long)tzOff * 3600L;
    struct tm lt;
    gmtime_r(&epoch, &lt);
    localH = lt.tm_hour; localMin = lt.tm_min; localSec = lt.tm_sec;
    localY = lt.tm_year + 1900; localMon = lt.tm_mon + 1; localD = lt.tm_mday;
    localDateValid = true;
  } else {
    localH = (utcH + tzOff + 24) % 24;
  }

  snprintf(buf, sizeof(buf), "%02d:%02d:%02d", localH, localMin, localSec);
  xCentered(buf, 10, TFT_GREEN, F_SEG48);

  if (localDateValid) snprintf(buf, sizeof(buf), "%04d-%02d-%02d", localY, localMon, localD);
  else                snprintf(buf, sizeof(buf), "----/--/--");
  xCentered(buf, 70, TFT_WHITE, F_MED);

  if (localDateValid) {
    int y2 = localY, m2 = localMon, d2 = localD;
    if (m2 < 3) { m2 += 12; y2--; }
    int dow = (d2 + (13 * (m2 + 1)) / 5 + y2 + y2 / 4 - y2 / 100 + y2 / 400) % 7;
    const char* days[] = {"Sat", "Sun", "Mon", "Tue", "Wed", "Thu", "Fri"};
    if (hasLoc) snprintf(buf, sizeof(buf), "%s  UTC%+d%s", days[dow], tzOff, dstAdj ? " DST" : "");
    else        snprintf(buf, sizeof(buf), "%s  UTC", days[dow]);
  } else {
    if (hasLoc) snprintf(buf, sizeof(buf), "UTC%+d%s", tzOff, dstAdj ? " DST" : "");
    else        snprintf(buf, sizeof(buf), "UTC");
  }
  xCentered(buf, 110, TFT_LIGHTGREY, F_BODY);

  snprintf(buf, sizeof(buf), "UTC %02d:%02d:%02d", utcH, utcM, utcS);
  xCentered(buf, 136, TFT_DARKGREY, F_BODY);

  bool fixOk = ggaFixQuality >= 1;
  snprintf(buf, sizeof(buf), "GPS: %s", fixOk ? "Fix OK" : "No Fix");
  xCentered(buf, 164, fixOk ? TFT_GREEN : TFT_RED, F_BODY);

  uint32_t age = gps.location.age();
  if (age < 10000) snprintf(buf, sizeof(buf), "Age: %.1fs", age / 1000.0);
  else             snprintf(buf, sizeof(buf), "Age: >10s");
  xCentered(buf, 188, TFT_DARKGREY, F_BODY);

  if (imuAvailable) {
    snprintf(buf, sizeof(buf), "%.1fC", imuTemp);
    xCentered(buf, 212, TFT_DARKGREY, F_BODY);
  }
}

// ------------------------------------------------------------------
//  Screen 15: Waypoint entry and navigation
// ------------------------------------------------------------------
void drawWaypointExt() {
  char buf[48];
  if (waypointEditing) {
    xCentered("-- Enter Waypoint --", 8, TFT_GREEN, F_BODY);
    xCentered("Nav: [;]   Digits: [0-9 . -]   [ok] Save", 32, TFT_DARKGREY, F_SMALL);
    for (int f = 0; f < 2; f++) {
      snprintf(buf, sizeof(buf), "%s %s%s", f == 0 ? "Lat:" : "Lon:",
               waypointTmp[f].c_str(), waypointField == f ? "_" : "");
      int y = 70 + f * 46;
      if (waypointField == f) frameBuf.drawRect(10, y - 6, 300, 38, TFT_GREEN);
      xText(buf, 20, y, waypointField == f ? TFT_GREEN : TFT_WHITE, F_MED);
    }
    xText("Example: 40.689247 / -74.044502", 20, 178, TFT_DARKGREY, F_BODY);
    xText("[w] cancel     [DEL] backspace", 20, 204, TFT_DARKGREY, F_BODY);
    return;
  }

  if (!waypointSet) {
    xCentered("-- Waypoint Nav --", 10, TFT_GREEN, F_BODY);
    xCentered("No waypoint", 84, TFT_DARKGREY, F_MED);
    xCentered("Press [w] to set target", 140, TFT_WHITE, F_BODY);
    xCentered("Enter geocache lat/lon", 168, TFT_DARKGREY, F_BODY);
    return;
  }

  bool locValid = gps.location.isValid();
  bool courseValid = gps.course.isValid() && gps.speed.kmph() > 1.0;
  float curLat = gps.location.lat(), curLon = gps.location.lng();
  float bearing = bearingTo(curLat, curLon, waypointLat, waypointLon);
  float distKm = haversineKm(curLat, curLon, waypointLat, waypointLon);
  float heading = gps.course.deg();
  float relBearing = courseValid ? fmod(bearing - heading + 360.0, 360.0) : bearing;

  // Compass (left).
  const int cx = 104, cy = 120, cr = 92;
  frameBuf.drawCircle(cx, cy, cr, TFT_DARKGREY);
  frameBuf.drawCircle(cx, cy, cr + 1, TFT_DARKGREY);
  for (int a = 0; a < 360; a += 30) {           // ticks
    float ar = radians(a);
    int inner = (a % 90 == 0) ? cr - 10 : cr - 6;
    frameBuf.drawLine(cx + (int)(sin(ar) * inner), cy - (int)(cos(ar) * inner),
                      cx + (int)(sin(ar) * cr),    cy - (int)(cos(ar) * cr), TFT_DARKGREY);
  }
  xText("N", cx, cy - cr + 12, TFT_LIGHTGREY, F_SMALL, TC_DATUM);
  xText("S", cx, cy + cr - 12, TFT_LIGHTGREY, F_SMALL, BC_DATUM);
  xText("E", cx + cr - 12, cy, TFT_LIGHTGREY, F_SMALL, MR_DATUM);
  xText("W", cx - cr + 12, cy, TFT_LIGHTGREY, F_SMALL, ML_DATUM);

  if (locValid) {
    float angRad = radians(relBearing);
    float tipLen = cr - 24, tailLen = 26, wingLen = 22, spread = 0.42;
    int tipX = cx + (int)(sin(angRad) * tipLen);
    int tipY = cy - (int)(cos(angRad) * tipLen);
    int tailX = cx - (int)(sin(angRad) * tailLen);
    int tailY = cy + (int)(cos(angRad) * tailLen);
    int lwX = cx - (int)(sin(angRad - spread) * wingLen);
    int lwY = cy + (int)(cos(angRad - spread) * wingLen);
    int rwX = cx - (int)(sin(angRad + spread) * wingLen);
    int rwY = cy + (int)(cos(angRad + spread) * wingLen);
    frameBuf.fillTriangle(tipX, tipY, lwX, lwY, rwX, rwY, TFT_GREEN);
    frameBuf.drawLine(cx, cy, tailX, tailY, TFT_DARKGREY);
    frameBuf.drawLine(cx + 1, cy, tailX + 1, tailY, TFT_DARKGREY);
    if (courseValid) frameBuf.fillCircle(cx, cy - cr + 4, 3, TFT_GREEN);
  } else {
    xText("No fix", cx, cy, TFT_RED, F_BODY, MC_DATUM);
  }

  // Info panel (right).
  const int rx = 206;
  xText("DISTANCE", rx, 8, TFT_GREEN, F_BODY);
  if (locValid) {
    if (distKm < 1.0)        snprintf(buf, sizeof(buf), "%.0fm", distKm * 1000.0);
    else if (distKm < 100.0) snprintf(buf, sizeof(buf), "%.2fkm", distKm);
    else                     snprintf(buf, sizeof(buf), "%.1fkm", distKm);
  } else snprintf(buf, sizeof(buf), "---");
  xText(buf, rx, 28, TFT_WHITE, F_MED);

  xText("BEARING", rx, 68, TFT_GREEN, F_BODY);
  if (locValid) {
    char pre[8], post[6];
    snprintf(pre, sizeof(pre), "%.0f", bearing);
    snprintf(post, sizeof(post), "%s", cardinalFromHeading(bearing));
    xTextDeg(pre, post, rx, 88, TFT_WHITE, F_MED, 0);
  } else xText("---", rx, 88, TFT_WHITE, F_MED);

  snprintf(buf, sizeof(buf), "%.5f", waypointLat);
  xText(buf, rx, 130, TFT_DARKGREY, F_BODY);
  snprintf(buf, sizeof(buf), "%.5f", waypointLon);
  xText(buf, rx, 148, TFT_DARKGREY, F_BODY);
  xText(courseValid ? "Mode: Track up" : "Mode: North up", rx, 178, TFT_DARKGREY, F_SMALL);
  xText("[w] edit  [r] clear", rx, 204, TFT_DARKGREY, F_SMALL);
}

// ------------------------------------------------------------------
//  Screen dispatch (external display)
// ------------------------------------------------------------------
void drawCurrentScreenExt() {
  switch (currentScreen) {
    case SCR_MAIN:          drawMainExt();          break;
    case SCR_SKY_VIEW:      drawSkyViewExt();       break;
    case SCR_SIGNAL_BARS:   drawSignalBarsExt();    break;
    case SCR_FIX_SUMMARY:   drawFixSummaryExt();    break;
    case SCR_DASHBOARD:     drawDashboardExt();     break;
    case SCR_COORDINATES:   drawCoordinatesExt();   break;
    case SCR_BREADCRUMB:    drawBreadcrumbExt();    break;
    case SCR_ALT_PROFILE:   drawAltProfileExt();    break;
    case SCR_SPEED_GRAPH:   drawSpeedGraphExt();    break;
    case SCR_TRIP_STATS:    drawTripStatsExt();     break;
    case SCR_CONSTELLATION: drawConstellationExt(); break;
    case SCR_NMEA_MONITOR:  drawNmeaMonitorExt();   break;
    case SCR_GPS_CLOCK:     drawGpsClockExt();      break;
    case SCR_GPS_MAP:       drawScreenGpsMap();     break;   // shared, resolution-aware
    case SCR_3D_GLOBE:      drawScreen3DGlobe();    break;   // shared, resolution-aware
    case SCR_WAYPOINT:      drawWaypointExt();      break;
    default: break;
  }
}

// ------------------------------------------------------------------
//  Popups (external display)
// ------------------------------------------------------------------
static void popupBox(int x, int y, int w, int h) {
  frameBuf.fillRect(x - 2, y - 2, w + 4, h + 4, TFT_BLACK);
  frameBuf.drawRect(x, y, w, h, TFT_GREEN);
  frameBuf.drawRect(x + 1, y + 1, w - 2, h - 2, TFT_GREEN);
}

void drawHelpExt(const char* const* lines, int count) {
  popupBox(4, 4, screenW - 8, screenH - 8);
  int y = 11;
  for (int i = 0; i < count; i++) {
    const char* line = lines[i];
    const char* close = strchr(line, ']');
    if (close) {
      char key[12];
      int n = (int)(close - line) + 1;
      snprintf(key, sizeof(key), "%.*s", n, line);
      xText(key, 14, y, TFT_GREEN, F_BODY);
      xText(close + 1, 14 + 6 * 8, y, TFT_WHITE, F_BODY);
    } else {
      xText(line, 14, y, TFT_WHITE, F_BODY);
    }
    y += 17;
  }
}

void drawInfoExt() {
  popupBox(20, 30, 280, 180);
  char buf[48];
  int y = 42;
  xText("Cardputer ADV GPS Info", 34, y, TFT_GREEN, F_BODY);              y += 22;
  xText("V " APP_VERSION " (ADV) by alcor55", 34, y, TFT_WHITE, F_BODY);  y += 30;
  xText("Github:", 34, y, TFT_WHITE, F_BODY);                             y += 20;
  xText("https://github.com/alcor55", 34, y, TFT_LIGHTGREY, F_BODY);      y += 18;
  xText("/Cardputer-GPS-Info", 34, y, TFT_LIGHTGREY, F_BODY);             y += 30;
  snprintf(buf, sizeof(buf), "Display: external %dx%d", screenW, screenH);
  xText(buf, 34, y, TFT_CYAN, F_SMALL);
}

void drawConfigExt() {
  popupBox(10, 10, screenW - 20, screenH - 20);
  char buf[64];
  int x = 24, y = 22;
  xText("Configurations:", x, y, TFT_GREEN, F_BODY);                       y += 22;
  xText("Nav: [Up/Dow]. Val: [0-9].", x, y, TFT_WHITE, F_BODY);            y += 18;
  xText("Exit: [c]. Save: [ok].", x, y, TFT_WHITE, F_BODY);                y += 30;
  snprintf(buf, sizeof(buf), "RX pin (act:%d): %s %s", gpsRxPin, configsTmp[0].c_str(), configsMenuSel == 0 ? "<" : "");
  xText(buf, x, y, configsMenuSel == 0 ? TFT_GREEN : TFT_WHITE, F_BODY);  y += 22;
  snprintf(buf, sizeof(buf), "TX pin (act:%d): %s %s", gpsTxPin, configsTmp[1].c_str(), configsMenuSel == 1 ? "<" : "");
  xText(buf, x, y, configsMenuSel == 1 ? TFT_GREEN : TFT_WHITE, F_BODY);  y += 22;
  snprintf(buf, sizeof(buf), "Baud (act:%d): %s %s", gpsBaud, configsTmp[2].c_str(), configsMenuSel == 2 ? "<" : "");
  xText(buf, x, y, configsMenuSel == 2 ? TFT_GREEN : TFT_WHITE, F_BODY);  y += 32;
  frameBuf.drawFastHLine(x, y - 8, screenW - 2 * x, TFT_DARKGREY);
  snprintf(buf, sizeof(buf), "Display: external %dx%d", screenW, screenH);
  xText(buf, x, y, TFT_CYAN, F_SMALL);                                    y += 18;
  xText("Display + pins: edit " CONFIG_PATH " (SD)", x, y, TFT_DARKGREY, F_SMALL);
}

void drawSentExt() {
  int w = 140, h = 44;
  popupBox((screenW - w) / 2, (screenH - h) / 2, w, h);
  xText("Sent!", screenW / 2, screenH / 2, TFT_GREEN, F_MED, MC_DATUM);
}
