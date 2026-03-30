// WeatherApp.cpp
// Build: g++ -std=c++17 WeatherApp.cpp imgui/*.cpp imgui/backends/imgui_impl_glfw.cpp
//            imgui/backends/imgui_impl_opengl3.cpp -lGL -lglfw -o WeatherApp

#include <windows.h>
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#include "GLFW/include/GLFW/glfw3.h"
#include <cmath>
#include <vector>
#include <string>
#include <algorithm>

// ─── Math helpers ────────────────────────────────────────────────────────────
static float Lerp(float a, float b, float t) { return a + (b - a) * t; }
static float EaseOut(float t) { return 1.f - (1.f - t) * (1.f - t); }

// ─── Data ────────────────────────────────────────────────────────────────────
enum Condition { SUNNY, PARTLY_CLOUDY, CLOUDY, RAINY, STORMY, SNOWY, FOGGY };

struct ForecastDay { const char* day; Condition cond; int hi, lo; };

struct City {
    const char* name;
    int temp, feels, humidity, wind, uv, aqi;
    const char* aqiLabel;
    Condition cond;
    ForecastDay forecast[5];
    int hourly[10];
};

static const City Cities[] = {
    { "London", 22, 20, 68, 14, 4, 22, "Good", PARTLY_CLOUDY,
      {{"Mon",PARTLY_CLOUDY,22,15},{"Tue",RAINY,19,13},{"Wed",STORMY,17,12},{"Thu",CLOUDY,21,14},{"Fri",SUNNY,25,16}},
      {14,15,17,19,22,22,21,19,17,15} },

    { "Tokyo", 31, 34, 82, 8, 8, 55, "Moderate", SUNNY,
      {{"Mon",SUNNY,31,24},{"Tue",PARTLY_CLOUDY,29,23},{"Wed",RAINY,28,22},{"Thu",SUNNY,30,23},{"Fri",SUNNY,32,25}},
      {24,25,27,29,31,31,30,29,27,25} },

    { "Dubai", 42, 45, 35, 22, 11, 80, "Moderate", SUNNY,
      {{"Mon",SUNNY,42,30},{"Tue",SUNNY,41,29},{"Wed",SUNNY,43,31},{"Thu",PARTLY_CLOUDY,40,28},{"Fri",PARTLY_CLOUDY,39,27}},
      {30,32,35,38,41,42,42,41,38,34} },

    { "NYC", 8, 5, 55, 30, 2, 18, "Good", SNOWY,
      {{"Mon",SNOWY,8,2},{"Tue",SNOWY,5,-1},{"Wed",CLOUDY,10,3},{"Thu",PARTLY_CLOUDY,14,6},{"Fri",SUNNY,16,8}},
      {2,3,4,6,8,8,7,6,5,3} },

    { "Mumbai", 28, 32, 90, 18, 6, 110, "Unhealthy", RAINY,
      {{"Mon",RAINY,28,23},{"Tue",STORMY,27,22},{"Wed",RAINY,29,24},{"Thu",PARTLY_CLOUDY,30,24},{"Fri",SUNNY,31,25}},
      {23,24,25,27,28,28,27,26,25,24} },

    { "Reykjavik", -2, -7, 78, 45, 1, 8, "Good", FOGGY,
      {{"Mon",FOGGY,-2,-6},{"Tue",SNOWY,-4,-9},{"Wed",CLOUDY,-1,-5},{"Thu",PARTLY_CLOUDY,2,-3},{"Fri",FOGGY,1,-4}},
      {-6,-5,-4,-3,-2,-2,-3,-4,-5,-6} },
};
static const int CityCount = 6;

// ─── Theme ───────────────────────────────────────────────────────────────────
void ApplyDarkTheme() {
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding = s.ChildRounding = s.PopupRounding = 8.f;
    s.FrameRounding = s.GrabRounding = 4.f;
    s.WindowPadding = {12, 12};
    s.FramePadding  = {8, 5};
    s.ItemSpacing   = {8, 6};

    ImVec4* c = s.Colors;
    c[ImGuiCol_WindowBg]         = {0.11f, 0.11f, 0.18f, 1.f};
    c[ImGuiCol_ChildBg]          = {0.13f, 0.13f, 0.20f, 1.f};
    c[ImGuiCol_FrameBg]          = {0.17f, 0.17f, 0.27f, 1.f};
    c[ImGuiCol_FrameBgHovered]   = {0.22f, 0.22f, 0.33f, 1.f};
    c[ImGuiCol_Button]           = {0.24f, 0.24f, 0.38f, 1.f};
    c[ImGuiCol_ButtonHovered]    = {0.30f, 0.30f, 0.46f, 1.f};
    c[ImGuiCol_ButtonActive]     = {0.20f, 0.20f, 0.32f, 1.f};
    c[ImGuiCol_Header]           = {0.38f, 0.29f, 0.62f, 0.5f};
    c[ImGuiCol_HeaderHovered]    = {0.38f, 0.29f, 0.62f, 0.8f};
    c[ImGuiCol_HeaderActive]     = {0.38f, 0.29f, 0.62f, 1.f};
    c[ImGuiCol_Separator]        = {0.22f, 0.22f, 0.30f, 1.f};
    c[ImGuiCol_Text]             = {0.80f, 0.84f, 0.96f, 1.f};
    c[ImGuiCol_TextDisabled]     = {0.38f, 0.42f, 0.60f, 1.f};
    c[ImGuiCol_Tab]              = {0.14f, 0.14f, 0.22f, 1.f};
    c[ImGuiCol_TabHovered]       = {0.38f, 0.29f, 0.62f, 0.8f};
    c[ImGuiCol_TabActive]        = {0.11f, 0.11f, 0.18f, 1.f};
    c[ImGuiCol_TitleBgActive]    = {0.18f, 0.18f, 0.26f, 1.f};
    c[ImGuiCol_SliderGrab]       = {0.74f, 0.58f, 0.98f, 1.f};
    c[ImGuiCol_SliderGrabActive] = {0.85f, 0.72f, 0.99f, 1.f};
    c[ImGuiCol_CheckMark]        = {0.74f, 0.58f, 0.98f, 1.f};
    c[ImGuiCol_PlotHistogram]    = {0.74f, 0.58f, 0.98f, 1.f};
    c[ImGuiCol_ScrollbarBg]      = {0.11f, 0.11f, 0.18f, 1.f};
    c[ImGuiCol_ScrollbarGrab]    = {0.24f, 0.24f, 0.38f, 1.f};
}

// ─── Sky gradient per condition ───────────────────────────────────────────────
static ImVec4 SkyTop(Condition c) {
    switch(c) {
        case SUNNY:        return {0.06f, 0.31f, 0.66f, 1.f};
        case PARTLY_CLOUDY:return {0.10f, 0.37f, 0.63f, 1.f};
        case CLOUDY:       return {0.24f, 0.35f, 0.41f, 1.f};
        case RAINY:        return {0.12f, 0.23f, 0.30f, 1.f};
        case STORMY:       return {0.05f, 0.08f, 0.13f, 1.f};
        case SNOWY:        return {0.29f, 0.38f, 0.45f, 1.f};
        case FOGGY:        return {0.35f, 0.42f, 0.47f, 1.f};
    }
    return {0.1f,0.3f,0.6f,1.f};
}
static ImVec4 SkyBot(Condition c) {
    switch(c) {
        case SUNNY:        return {0.36f, 0.64f, 0.96f, 1.f};
        case PARTLY_CLOUDY:return {0.45f, 0.72f, 0.91f, 1.f};
        case CLOUDY:       return {0.56f, 0.67f, 0.72f, 1.f};
        case RAINY:        return {0.27f, 0.40f, 0.48f, 1.f};
        case STORMY:       return {0.15f, 0.20f, 0.27f, 1.f};
        case SNOWY:        return {0.56f, 0.66f, 0.72f, 1.f};
        case FOGGY:        return {0.60f, 0.67f, 0.72f, 1.f};
    }
    return {0.4f,0.7f,0.95f,1.f};
}

// ─── Cloud drawing ────────────────────────────────────────────────────────────
// One cloud = a group of overlapping filled circles drawn directly on ImDrawList.
// We use AddCircleFilled with decreasing alpha toward the edges for a soft look.
struct CloudBlob { float dx, dy, r; };
static const CloudBlob kBlobTemplate[] = {
    {  0.f,  0.f, 38.f}, { 32.f,-16.f, 30.f}, { 62.f, -8.f, 34.f},
    { 90.f, -2.f, 28.f}, {116.f,  4.f, 24.f}, {-20.f,  6.f, 26.f},
    { 22.f,  8.f, 22.f}, { 54.f, 10.f, 20.f}, { 84.f,  8.f, 18.f},
};

void DrawCloud(ImDrawList* dl, ImVec2 pos, float scale, float alpha) {
    for (auto& b : kBlobTemplate) {
        ImVec2 c = { pos.x + b.dx * scale, pos.y + b.dy * scale };
        float  r = b.r * scale;
        // Soft core → transparent rim via 3 stacked circles
        dl->AddCircleFilled(c, r,
            ImGui::ColorConvertFloat4ToU32({1.f, 1.f, 1.f, alpha * 0.95f}));
        dl->AddCircleFilled(c, r * 0.72f,
            ImGui::ColorConvertFloat4ToU32({1.f, 1.f, 1.f, alpha * 0.12f}));
    }
}

// ─── Particle systems ─────────────────────────────────────────────────────────
struct Raindrop { float x, y, speed, len, opacity; };
struct Snowflake { float x, y, speed, r, drift, phase; };

struct ParticleSystem {
    std::vector<Raindrop>  rain;
    std::vector<Snowflake> snow;

    void Reset(Condition cond, float W, float H) {
        rain.clear(); snow.clear();
        if (cond == RAINY || cond == STORMY) {
            rain.resize(55);
            for (auto& d : rain) {
                d.x = (float)(rand() % (int)W);
                d.y = (float)(rand() % (int)H);
                d.speed   = 180.f + (rand() % 120);
                d.len     = 8.f   + (rand() % 10);
                d.opacity = 0.4f  + (rand() % 50) / 100.f;
            }
        }
        if (cond == SNOWY) {
            snow.resize(40);
            for (auto& s : snow) {
                s.x     = (float)(rand() % (int)W);
                s.y     = (float)(rand() % (int)H);
                s.speed  = 25.f  + (rand() % 35);
                s.r      = 1.5f  + (rand() % 30) / 10.f;
                s.drift  = (rand() % 200 - 100) / 100.f;
                s.phase  = (rand() % 628) / 100.f;
            }
        }
    }

    void Update(float dt, float W, float H, Condition cond, float t) {
        for (auto& d : rain) {
            d.y += d.speed * dt;
            d.x += (cond == STORMY) ? 60.f * dt : 0.f;
            if (d.y > H + 10) { d.y = -10; d.x = (float)(rand() % (int)W); }
        }
        for (auto& s : snow) {
            s.y += s.speed * dt;
            s.x += sinf(t * 0.8f + s.phase) * s.drift * dt * 30.f;
            if (s.y > H + 10) { s.y = -10; s.x = (float)(rand() % (int)W); }
        }
    }

    void Draw(ImDrawList* dl, ImVec2 origin, Condition cond) {
        for (auto& d : rain) {
            float angle = (cond == STORMY) ? 0.2f : 0.03f;
            ImVec2 a = { origin.x + d.x,            origin.y + d.y };
            ImVec2 b = { origin.x + d.x + angle * d.len, origin.y + d.y + d.len };
            dl->AddLine(a, b,
                ImGui::ColorConvertFloat4ToU32({0.63f,0.78f,0.94f, d.opacity}), 1.3f);
        }
        for (auto& s : snow) {
            dl->AddCircleFilled(
                { origin.x + s.x, origin.y + s.y },
                s.r,
                ImGui::ColorConvertFloat4ToU32({1.f,1.f,1.f,0.88f}));
        }
    }
};

// ─── Animated value helper ─────────────────────────────────────────────────────
struct AnimFloat {
    float cur, target;
    void Set(float v)    { target = v; }
    void Tick(float dt, float sp = 6.f) { cur += (target - cur) * std::min(sp * dt, 1.f); }
};

// ─── Main app state ────────────────────────────────────────────────────────────
struct WeatherApp {
    int           selectedCity  = 0;
    int           prevCity      = -1;
    bool          useFahrenheit = false;
    float         cloudOffset   = 0.f;
    float         cloudOffset2  = 0.f;
    float         appTime       = 0.f;
    ParticleSystem particles;
    AnimFloat     tempAnim, feelsAnim;
    float         lightningTimer = 0.f;
    bool          lightningFlash = false;

    void SelectCity(int i) {
        if (i == selectedCity) return;
        selectedCity = i;
        particles.Reset(Cities[i].cond, 720, 200);
        tempAnim.Set((float)Cities[i].temp);
        feelsAnim.Set((float)Cities[i].feels);
        lightningTimer = 0.f;
    }

    float FmtTemp(int c) {
        return useFahrenheit ? (float)(c * 9 / 5 + 32) : (float)c;
    }

    const char* TempUnit() { return useFahrenheit ? "°F" : "°C"; }

    // Sky gradient drawn as a vertical quad via AddRectFilledMultiColor
    void DrawSkyGradient(ImDrawList* dl, ImVec2 p, ImVec2 sz, Condition cond) {
        ImVec4 top = SkyTop(cond), bot = SkyBot(cond);
        dl->AddRectFilledMultiColor(p, {p.x+sz.x, p.y+sz.y},
            ImGui::ColorConvertFloat4ToU32(top),
            ImGui::ColorConvertFloat4ToU32(top),
            ImGui::ColorConvertFloat4ToU32(bot),
            ImGui::ColorConvertFloat4ToU32(bot));
    }

    // Sun with animated rays
    void DrawSun(ImDrawList* dl, ImVec2 origin, float t, Condition cond) {
        float sx = origin.x + (cond == SUNNY ? 90.f : 120.f);
        float sy = origin.y + (cond == SUNNY ? 55.f : 60.f);
        float sr = cond == SUNNY ? 36.f : 26.f;
        int  rays = cond == SUNNY ? 12 : 8;
        float baseAlpha = cond == SUNNY ? 0.55f : 0.38f;

        for (int i = 0; i < rays; i++) {
            float a = (IM_PI * 2.f / rays) * i;
            float pulse = sinf(t * 1.8f + i * 0.6f) * 0.5f + 0.5f;
            float rayAlpha = baseAlpha * (0.6f + 0.4f * pulse);
            float inner = sr + 6.f, outer = sr + (cond == SUNNY ? 22.f : 15.f);
            outer += pulse * 4.f;
            ImVec2 p1 = { sx + cosf(a) * inner, sy + sinf(a) * inner };
            ImVec2 p2 = { sx + cosf(a) * outer, sy + sinf(a) * outer };
            dl->AddLine(p1, p2,
                ImGui::ColorConvertFloat4ToU32({1.f, 0.92f, 0.4f, rayAlpha}), 3.f);
        }
        // Core glow ring
        dl->AddCircleFilled({sx,sy}, sr + 6.f,
            ImGui::ColorConvertFloat4ToU32({1.f,0.90f,0.35f,0.25f}));
        dl->AddCircleFilled({sx,sy}, sr,
            ImGui::ColorConvertFloat4ToU32({1.f,0.87f,0.25f,0.95f}));
        dl->AddCircleFilled({sx,sy}, sr * 0.6f,
            ImGui::ColorConvertFloat4ToU32({1.f,0.96f,0.72f,0.7f}));
    }

    void DrawFog(ImDrawList* dl, ImVec2 origin, ImVec2 sz, float t) {
        for (int i = 0; i < 5; i++) {
            float off = fmodf(t * 10.f + i * 60.f, sz.x + 100.f);
            float fy  = origin.y + 60.f + i * 28.f;
            float alpha = 0.12f + i * 0.018f;
            float fw = 200.f + i * 40.f;
            dl->AddRectFilledMultiColor(
                { origin.x + off - fw, fy - 12.f },
                { origin.x + off,       fy + 12.f },
                IM_COL32(200,215,230,0),
                ImGui::ColorConvertFloat4ToU32({0.78f,0.84f,0.90f,alpha}),
                ImGui::ColorConvertFloat4ToU32({0.78f,0.84f,0.90f,alpha}),
                IM_COL32(200,215,230,0));
        }
    }

    void DrawSkyScene(ImDrawList* dl, ImVec2 origin, ImVec2 sz, Condition cond, float t, float dt) {
        dl->PushClipRect(origin, {origin.x+sz.x, origin.y+sz.y}, true);

        DrawSkyGradient(dl, origin, sz, cond);

        if (cond == SUNNY || cond == PARTLY_CLOUDY)
            DrawSun(dl, origin, t, cond);

        // Cloud scroll — two layers at different speeds
        cloudOffset  = fmodf(cloudOffset  + dt * (cond == STORMY ? 55 : 22), sz.x + 300.f);
        cloudOffset2 = fmodf(cloudOffset2 + dt * (cond == STORMY ? 32 : 14), sz.x + 300.f);

        struct CloudDef { float x, y, scale, alpha; int layer; };
        static const CloudDef defs[] = {
            {   0,120,.55f,.30f,0},{200,105,.72f,.34f,0},{420,112,.62f,.32f,0},{600,118,.50f,.28f,0},
            {  30,138,.60f,.22f,1},{240,128,.68f,.26f,1},{460,132,.58f,.24f,1},{660,135,.52f,.20f,1},
            { 100, 90,.80f,.50f,0},{300, 80,.90f,.55f,0},{520, 86,.75f,.48f,0},{720, 92,.70f,.45f,0},
        };

        int nClouds = (cond==SUNNY)?4 : (cond==PARTLY_CLOUDY)?8 : 12;
        for (int i = 0; i < nClouds && i < 12; i++) {
            float off  = defs[i].layer == 0 ? cloudOffset : cloudOffset2;
            float cx   = fmodf(defs[i].x - off + sz.x * 3, sz.x + 300.f) - 150.f;
            float alpha = defs[i].alpha *
                (cond==SUNNY      ? 0.38f :
                 cond==PARTLY_CLOUDY? 0.80f :
                 cond==CLOUDY     ? 1.00f :
                 cond==RAINY      ? 0.92f :
                 cond==STORMY     ? 0.70f :
                 cond==SNOWY      ? 0.80f : 0.55f);
            DrawCloud(dl, {origin.x + cx, origin.y + defs[i].y}, defs[i].scale, alpha);
        }

        particles.Update(dt, sz.x, sz.y, cond, t);
        particles.Draw(dl, origin, cond);

        if (cond == FOGGY)   DrawFog(dl, origin, sz, t);

        // Lightning flash
        if (cond == STORMY) {
            lightningTimer -= dt;
            if (lightningTimer <= 0.f) {
                lightningTimer = 3.f + (rand() % 500) / 100.f;
                lightningFlash = true;
            }
            if (lightningFlash) {
                float a = sinf(appTime * 40.f) > 0.3f ? 0.35f : 0.f;
                dl->AddRectFilled(origin, {origin.x+sz.x,origin.y+sz.y},
                    ImGui::ColorConvertFloat4ToU32({0.7f,0.75f,1.f,a}));
                if (a < 0.01f) lightningFlash = false;
            }
        }

        dl->PopClipRect();
    }

    // Temp color: blue=cold, green=mild, amber=warm, orange=hot
    ImVec4 TempColor(int t) {
        if (t < 0)  return {0.58f,0.76f,0.99f,1.f};
        if (t < 10) return {0.38f,0.64f,0.98f,1.f};
        if (t < 20) return {0.20f,0.83f,0.60f,1.f};
        if (t < 30) return {0.98f,0.74f,0.26f,1.f};
        return {0.98f,0.40f,0.10f,1.f};
    }

    void Render(float dt) {
        appTime += dt;
        tempAnim.Tick(dt);
        feelsAnim.Tick(dt);

        const City& city = Cities[selectedCity];

        ImGui::SetNextWindowSize({780, 580}, ImGuiCond_Always);
        ImGui::SetNextWindowPos({40, 40}, ImGuiCond_FirstUseEver);
        ImGui::Begin("Weather", nullptr,
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 winPos  = ImGui::GetWindowPos();

        // ── Sidebar ────────────────────────────────────────────────────────
        ImGui::BeginChild("##sidebar", {130, -1}, true);
        ImGui::TextDisabled("CITIES");
        ImGui::Separator();
        for (int i = 0; i < CityCount; i++) {
            bool sel = (i == selectedCity);
            if (sel) {
                ImGui::PushStyleColor(ImGuiCol_Header,        {0.24f,0.18f,0.40f,1.f});
                ImGui::PushStyleColor(ImGuiCol_HeaderHovered, {0.28f,0.22f,0.46f,1.f});
                ImGui::PushStyleColor(ImGuiCol_Text,          {0.74f,0.58f,0.98f,1.f});
            }
            if (ImGui::Selectable(Cities[i].name, sel, 0, {0,26}))
                SelectCity(i);
            if (sel) ImGui::PopStyleColor(3);
        }
        ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
        ImGui::TextDisabled("UNIT");
        if (ImGui::Button(useFahrenheit ? "Fahrenheit" : "Celsius", {114,0}))
            useFahrenheit = !useFahrenheit;
        ImGui::EndChild();

        ImGui::SameLine();

        // ── Right panel ───────────────────────────────────────────────────
        ImGui::BeginGroup();

        // Sky scene — drawn on the ImDrawList directly
        ImVec2 skyPos = ImGui::GetCursorScreenPos();
        ImVec2 skySz  = {ImGui::GetContentRegionAvail().x, 200.f};
        DrawSkyScene(dl, skyPos, skySz, city.cond, appTime, dt);

        // Overlay text on sky
        {
            char buf[64];
            snprintf(buf, sizeof(buf), "%d%s", (int)FmtTemp(city.temp), TempUnit());
            ImVec2 ts = ImGui::CalcTextSize(buf);
            ImGui::SetCursorScreenPos({skyPos.x + skySz.x*.5f - ts.x*.5f, skyPos.y + skySz.y*.5f - 40.f});
            ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]); // Large font (see font setup below)
            ImGui::TextColored({1,1,1,1}, "%s", buf);
            ImGui::PopFont();
        }
        // Skip the sky rect in layout
        ImGui::Dummy(skySz);

        // ── Stats row ─────────────────────────────────────────────────────
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.f);
        ImGui::BeginChild("##stats", {-1, 52}, true,
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        float colW = ImGui::GetContentRegionAvail().x / 4.f;
        auto Stat = [&](const char* val, const char* label, ImVec4 col) {
            ImGui::BeginGroup();
            ImGui::TextColored(col, "%s", val);
            ImGui::TextDisabled("%s", label);
            ImGui::EndGroup();
            ImGui::SameLine(0, 0);
            ImGui::Dummy({colW - ImGui::GetItemRectSize().x, 0});
        };
        char buf[32];
        snprintf(buf, 32, "%d%s", (int)FmtTemp(city.feels), TempUnit());
        Stat(buf, "feels like", {0.80f,0.84f,0.96f,1.f});
        snprintf(buf, 32, "%d%%",  city.humidity);
        Stat(buf, "humidity",   {0.35f,0.82f,0.95f,1.f});
        snprintf(buf, 32, "%d km/h", city.wind);
        Stat(buf, "wind",       {0.20f,0.90f,0.50f,1.f});
        snprintf(buf, 32, "UV %d", city.uv);
        Stat(buf, "uv index",   {1.00f,0.72f,0.42f,1.f});
        ImGui::EndChild();
        ImGui::PopStyleVar();

        // ── 5-Day Forecast ────────────────────────────────────────────────
        ImGui::Spacing();
        ImGui::TextDisabled("5-DAY FORECAST");
        ImGui::BeginChild("##forecast", {-1, 70}, true,
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        float fw = ImGui::GetContentRegionAvail().x / 5.f;
        static const char* condNames[] = {"Sunny","~Cloudy","Cloudy","Rainy","Stormy","Snowy","Foggy"};
        for (int i = 0; i < 5; i++) {
            const ForecastDay& f = city.forecast[i];
            ImGui::BeginGroup();
            ImGui::TextDisabled("%s", f.day);
            ImGui::TextColored(TempColor(f.hi), "%d°", (int)FmtTemp(f.hi));
            ImGui::TextColored({0.38f,0.42f,0.60f,1.f}, "%d°", (int)FmtTemp(f.lo));
            ImGui::EndGroup();
            ImGui::SameLine(0, fw - 40.f);
        }
        ImGui::EndChild();

        // ── Hourly bars ───────────────────────────────────────────────────
        ImGui::Spacing();
        ImGui::TextDisabled("HOURLY");
        ImGui::BeginChild("##hourly", {-1, 80}, true,
            ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        float barW  = 10.f, barGap = 28.f;
        float maxH  = 44.f;
        int maxT = *std::max_element(city.hourly, city.hourly+10);
        int minT = *std::min_element(city.hourly, city.hourly+10);
        int rng  = std::max(maxT - minT, 1);
        const char* hrs[] = {"Now","10","11","12","1p","2p","3p","4p","5p","6p"};
        ImVec2 base = ImGui::GetCursorScreenPos();
        for (int i = 0; i < 10; i++) {
            float t   = (float)(city.hourly[i] - minT) / rng;
            float h   = 8.f + t * maxH;
            ImVec4 bc = TempColor(city.hourly[i]);
            ImVec2 br = {base.x + i * barGap,         base.y + 50.f};
            ImVec2 tl = {br.x,                         br.y - h};
            dl->AddRectFilled(tl, {br.x+barW, br.y},
                ImGui::ColorConvertFloat4ToU32(bc), 3.f);
            ImGui::SetCursorScreenPos({tl.x - 2.f, base.y});
            ImGui::TextDisabled("%s", hrs[i]);
        }
        ImGui::Dummy({10 * barGap, 58.f});
        ImGui::EndChild();

        // ── AQI track ─────────────────────────────────────────────────────
        ImGui::Spacing();
        ImGui::TextDisabled("AIR QUALITY");
        ImGui::BeginChild("##aqi", {-1, 40}, false);
        float aqiPct = std::min((float)city.aqi / 200.f, 1.f);
        float trackW = ImGui::GetContentRegionAvail().x - 80.f;
        ImVec2 trackPos = ImGui::GetCursorScreenPos();
        // Rainbow gradient track
        dl->AddRectFilledMultiColor(
            trackPos, {trackPos.x+trackW, trackPos.y+8.f},
            IM_COL32(80,250,123,200), IM_COL32(255,80,80,200),
            IM_COL32(255,80,80,200), IM_COL32(80,250,123,200));
        // Dot
        float dotX = trackPos.x + aqiPct * trackW;
        dl->AddCircleFilled({dotX, trackPos.y+4.f}, 7.f,
            ImGui::ColorConvertFloat4ToU32({1,1,1,1}));
        dl->AddCircle({dotX, trackPos.y+4.f}, 7.f,
            ImGui::ColorConvertFloat4ToU32({0.1f,0.1f,0.18f,1.f}), 0, 2.f);
        ImGui::SetCursorScreenPos({trackPos.x + trackW + 12.f, trackPos.y - 4.f});
        ImGui::Text("%s  AQI %d", city.aqiLabel, city.aqi);
        ImGui::Dummy({0,12});
        ImGui::EndChild();

        ImGui::EndGroup();
        ImGui::End();
    }
};

// ─── Entry point ──────────────────────────────────────────────────────────────
int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* win = glfwCreateWindow(900, 700, "Weather", nullptr, nullptr);
    glfwMakeContextCurrent(win);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    // Load a large font for the temperature display (index 1)
    io.Fonts->AddFontDefault();
    // io.Fonts->AddFontFromFileTTF("Roboto-Light.ttf", 52.f); // index 1
    ImFontConfig cfg;
    cfg.SizePixels = 48.f;
    io.Fonts->AddFontDefault(&cfg);   // index 1 — big temp font
    ImGui_ImplGlfw_InitForOpenGL(win, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    ApplyDarkTheme();

    WeatherApp app;
    app.SelectCity(0);
    float lastTime = (float)glfwGetTime();

    while (!glfwWindowShouldClose(win)) {
        glfwPollEvents();
        float now = (float)glfwGetTime();
        float dt  = now - lastTime;
        lastTime  = now;

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        app.Render(dt);
        ImGui::Render();

        int W, H; glfwGetFramebufferSize(win, &W, &H);
        glViewport(0, 0, W, H);
        glClearColor(0.09f, 0.09f, 0.14f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(win);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(win);
    glfwTerminate();
}