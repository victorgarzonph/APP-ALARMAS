#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <cstring>
#include <ctime>
#include "Serial.h"

#include <string>
#include <vector>
#include <algorithm>
#include <sstream>

static const int   WINDOW_W       = 1280;
static const int   WINDOW_H       = 720;
static const int   TEMP_HISTORY   = 120;
static const float TEMP_MAX       = 100.f;
static const float TEMP_DANGER    = 30.0f;
static const float NIVEL_MIN      = 20.f;
static const float NIVEL_MAX      = 80.f;
static const float PRESION_WARN   = 0.65f;
static const float PRESION_DANGER = 45.0f;
static const int   MAX_ALERTS     = 100;

namespace Colors {
    const ImVec4 BgDark        = ImVec4(0.08f, 0.09f, 0.11f, 1.f);
    const ImVec4 Panel         = ImVec4(0.12f, 0.13f, 0.16f, 1.f);
    const ImVec4 PanelBorder   = ImVec4(0.22f, 0.25f, 0.30f, 1.f);
    const ImVec4 TextPrimary   = ImVec4(0.90f, 0.93f, 0.96f, 1.f);
    const ImVec4 TextMuted     = ImVec4(0.50f, 0.55f, 0.62f, 1.f);
    const ImVec4 TextAccent    = ImVec4(0.35f, 0.75f, 0.95f, 1.f);

    const ImVec4 Normal        = ImVec4(0.20f, 0.75f, 0.45f, 1.f);
    const ImVec4 Warning       = ImVec4(0.95f, 0.78f, 0.10f, 1.f);
    const ImVec4 Danger        = ImVec4(0.95f, 0.25f, 0.25f, 1.f);

    const ImVec4 BtnNormal     = ImVec4(0.18f, 0.22f, 0.30f, 1.f);
    const ImVec4 BtnHover      = ImVec4(0.25f, 0.32f, 0.45f, 1.f);
    const ImVec4 BtnActive     = ImVec4(0.30f, 0.40f, 0.58f, 1.f);
    const ImVec4 BtnDanger     = ImVec4(0.55f, 0.10f, 0.10f, 1.f);
    const ImVec4 BtnDangerHov  = ImVec4(0.75f, 0.15f, 0.15f, 1.f);
    const ImVec4 BtnDangerAct  = ImVec4(0.90f, 0.20f, 0.20f, 1.f);
    const ImVec4 BtnQuery      = ImVec4(0.12f, 0.32f, 0.50f, 1.f);
    const ImVec4 BtnQueryHov   = ImVec4(0.18f, 0.42f, 0.65f, 1.f);
    const ImVec4 BtnQueryAct   = ImVec4(0.22f, 0.52f, 0.78f, 1.f);
}

struct AlertEntry {
    std::string message;
    std::string timestamp;
    bool        acknowledged;
    std::string type;
};

// Variables globales
float temperatura = 0.f;
float nivel = 0.f;
float presion = 0.f;
float tempHistory[TEMP_HISTORY] = {};

std::vector<AlertEntry> alerts;
int unacknowledgedCount = 0;

std::string lastCommandFeedback = "";
time_t lastCommandTime = 0;

bool showStateWindow = false;
std::string rawStateText = "";


//  UTILIDADES

std::string GetTimestamp() {
    time_t now = time(nullptr);
    char buf[20];
    strftime(buf, sizeof(buf), "%H:%M:%S", localtime(&now));
    return std::string(buf);
}

ImVec4 GetNivelColor(float value) {
    if (value > NIVEL_MAX) return Colors::Danger;
    if (value < NIVEL_MIN) return Colors::Warning;
    return Colors::Normal;
}

const char* GetNivelStatus(float value) {
    if (value > NIVEL_MAX) return "PELIGRO - ALTO";
    if (value < NIVEL_MIN) return "BAJO";
    return "NORMAL";
}

void DrawSmartBar(const char* label, float value, float maxVal,
                  float warnThresh, float dangerThresh,
                  const char* unit = "%", bool isNivel = false)
{
    float fraction = value / maxVal;
    if (fraction > 1.f) fraction = 1.f;
    if (fraction < 0.f) fraction = 0.f;

    ImVec4 barColor = isNivel
        ? GetNivelColor(value)
        : (fraction >= dangerThresh ? Colors::Danger
          : (fraction >= warnThresh ? Colors::Warning : Colors::Normal));

    const char* status = isNivel
        ? GetNivelStatus(value)
        : (fraction >= dangerThresh ? "PELIGRO"
          : (fraction >= warnThresh ? "ALERTA" : "NORMAL"));

    ImGui::TextColored(Colors::TextMuted, "%s", label);
    ImGui::SameLine(170.f);
    ImGui::TextColored(barColor, "[%s]", status);

    char overlay[50];
    snprintf(overlay, sizeof(overlay), "%.1f %s", value, unit);

    ImVec2 pos = ImGui::GetCursorScreenPos();
    float w    = ImGui::GetContentRegionAvail().x;
    float h    = 28.f;

    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(pos, ImVec2(pos.x + w, pos.y + h), IM_COL32(25,27,33,255), 4.f);
    dl->AddRectFilled(pos, ImVec2(pos.x + w * fraction, pos.y + h),
                      IM_COL32((int)(barColor.x*255),(int)(barColor.y*255),(int)(barColor.z*255),230), 4.f);
    dl->AddRect(pos, ImVec2(pos.x + w, pos.y + h), IM_COL32(60,65,80,255), 4.f);

    ImVec2 ts = ImGui::CalcTextSize(overlay);
    dl->AddText(ImVec2(pos.x + (w - ts.x)*0.5f, pos.y + (h - ts.y)*0.5f),
                IM_COL32(240,245,255,255), overlay);

    ImGui::Dummy(ImVec2(w, h + 8));
}

//  VENTANA DE ESTADO - Dibuja la barra de nivel
void DrawMiniBar(float fraction, ImVec4 color) {
    ImVec2 pos = ImGui::GetCursorScreenPos();
    float w    = ImGui::GetContentRegionAvail().x;
    float h    = 20.f;

    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(pos, ImVec2(pos.x + w, pos.y + h), IM_COL32(20,22,28,255), 3.f);
    dl->AddRectFilled(pos, ImVec2(pos.x + w * fraction, pos.y + h),
                      IM_COL32((int)(color.x*255),(int)(color.y*255),(int)(color.z*255), 200), 3.f);
    dl->AddRect(pos, ImVec2(pos.x + w, pos.y + h), IM_COL32(55,60,75,255), 3.f);
    ImGui::Dummy(ImVec2(w, h + 4));
}


//  VENTANA DE ESTADO - Dibuja el contenido detallado del estado de las alarmas
void DisplayAlarmState(const std::string& rawLine) {
    // --- Limpiar \r y \n residuales del serial ---
    std::string line = rawLine;
    line.erase(std::remove_if(line.begin(), line.end(),
        [](char c){ return c == '\r' || c == '\n'; }), line.end());

    static const float COL_LABEL = 200.f;

    // Helper lambda: fila etiqueta + valor
    auto Row = [&](const char* label, auto drawValue) {
        ImGui::TextColored(Colors::TextMuted, "%s", label);
        ImGui::SameLine(COL_LABEL);
        drawValue();
    };

    //TEMPERATURA
    if (line.find("TIPO:TEMP") != std::string::npos) {
        int   pin = 0, act = 0;
        float val = 0.f, umbral = 0.f;
        int parsed = sscanf(line.c_str(),
            "TIPO:TEMP;PIN:%d;VAL:%f;UM:%f;ACT:%d",
            &pin, &val, &umbral, &act);

        bool   enPeligro  = (val > TEMP_DANGER);
        ImVec4 valColor   = enPeligro ? Colors::Danger : Colors::Normal;
        ImVec4 actColor   = act       ? Colors::Danger : Colors::Normal;

        // Encabezado con icono de estado
        ImGui::TextColored(Colors::TextAccent, "  TEMPERATURA");
        ImGui::SameLine(COL_LABEL);
        ImGui::TextColored(valColor, "%s", enPeligro ? "[PELIGRO]" : "[NORMAL]");

        ImGui::Spacing();

        Row("Valor actual:", [&](){
            ImGui::TextColored(valColor, "%.2f °C", val);
        });
        Row("Umbral de alarma:", [&](){
            ImGui::Text("%.2f °C", umbral);
        });
        Row("PIN del sensor:", [&](){
            ImGui::Text("%d", pin);
        });
        Row("Estado alarma:", [&](){
            ImGui::TextColored(actColor, "%s", act ? "ACTIVA" : "INACTIVA");
        });

        //Barra de progreso visual (sobre TEMP_MAX = 100)
        ImGui::Spacing();
        float frac = val / TEMP_MAX;
        if (frac > 1.f) frac = 1.f;
        DrawMiniBar(frac, valColor);
    }

    //NIVEL
    else if (line.find("TIPO:NIVEL") != std::string::npos) {
        int   pin = 0, act = 0;
        float val = 0.f, minv = 0.f, maxv = 0.f;
        int parsed = sscanf(line.c_str(),
            "TIPO:NIVEL;PIN:%d;VAL:%f;MIN:%f;MAX:%f;ACT:%d",
            &pin, &val, &minv, &maxv, &act);

        ImVec4      valColor  = GetNivelColor(val);
        const char* nivelSts  = GetNivelStatus(val);
        ImVec4      actColor  = act ? Colors::Danger : Colors::Normal;

        ImGui::TextColored(Colors::TextAccent, "  NIVEL DE TANQUE");
        ImGui::SameLine(COL_LABEL);
        ImGui::TextColored(valColor, "[%s]", nivelSts);

        ImGui::Spacing();

        Row("Valor actual:", [&](){
            ImGui::TextColored(valColor, "%.2f %%", val);
        });
        Row("Rango seguro:", [&](){
            ImGui::Text("%.1f %% - %.1f %%", minv, maxv);
        });
        Row("PIN del sensor:", [&](){
            ImGui::Text("%d", pin);
        });
        Row("Estado alarma:", [&](){
            ImGui::TextColored(actColor, "%s", act ? "ACTIVA" : "INACTIVA");
        });

        // Barra de nivel con marcas de rango
        ImGui::Spacing();
        float frac = val / 100.f;
        if (frac > 1.f) frac = 1.f;
        if (frac < 0.f) frac = 0.f;
        DrawMiniBar(frac, valColor);

        // Indicadores textuales del rango
        ImGui::TextColored(Colors::TextMuted, "MIN %.0f%%", minv);
        ImGui::SameLine();
        float barW = ImGui::GetContentRegionAvail().x;
        // Posicionar "MAX" al lado derecho aproximado
        char maxStr[16]; snprintf(maxStr, sizeof(maxStr), "MAX %.0f%%", maxv);
        ImVec2 maxSz = ImGui::CalcTextSize(maxStr);
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - maxSz.x + ImGui::GetCursorPosX());
        ImGui::TextColored(Colors::TextMuted, "%s", maxStr);
    }

    //PRESIÓN
    else if (line.find("TIPO:PRES") != std::string::npos) {
        int   pin = 0, act = 0;
        float val = 0.f, umbral = 0.f;
        int parsed = sscanf(line.c_str(),
            "TIPO:PRES;PIN:%d;VAL:%f;UM:%f;ACT:%d",
            &pin, &val, &umbral, &act);

        bool   enPeligro = (val > umbral);
        ImVec4 valColor  = enPeligro ? Colors::Danger : Colors::Normal;
        ImVec4 actColor  = act       ? Colors::Danger : Colors::Normal;

        ImGui::TextColored(Colors::TextAccent, "  PRESIÓN");
        ImGui::SameLine(COL_LABEL);
        ImGui::TextColored(valColor, "%s", enPeligro ? "[PELIGRO]" : "[NORMAL]");

        ImGui::Spacing();

        Row("Valor actual:", [&](){
            ImGui::TextColored(valColor, "%.2f PSI", val);
        });
        Row("Umbral de alarma:", [&](){
            ImGui::Text("%.2f PSI", umbral);
        });
        Row("PIN del sensor:", [&](){
            ImGui::Text("%d", pin);
        });
        Row("Estado alarma:", [&](){
            ImGui::TextColored(actColor, "%s", act ? "ACTIVA" : "INACTIVA");
        });

        // Barra de presión (sobre umbral*2 como referencia visual)
        ImGui::Spacing();
        float ref  = umbral > 0.f ? umbral * 2.f : 100.f;
        float frac = val / ref;
        if (frac > 1.f) frac = 1.f;
        DrawMiniBar(frac, valColor);
    }
}

// UI
bool StyledButton(const char* label, ImVec4 col, ImVec4 hov, ImVec4 act, float width = 0.f) {
    ImGui::PushStyleColor(ImGuiCol_Button,        col);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hov);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  act);
    bool clicked = ImGui::Button(label, ImVec2(width, 30.f));
    ImGui::PopStyleColor(3);
    return clicked;
}

void ApplyIndustrialStyle() {
    ImGuiStyle& s    = ImGui::GetStyle();
    s.WindowRounding = 6.f;
    s.FrameRounding  = 4.f;
    s.ItemSpacing    = ImVec2(8.f, 6.f);
    s.FramePadding   = ImVec2(8.f, 5.f);
    s.WindowPadding  = ImVec2(12.f, 12.f);
}


//  MAIN
int main() {
    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    GLFWwindow* window = glfwCreateWindow(WINDOW_W, WINDOW_H,
        "Sistema de Monitoreo Industrial | ESP32", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    ApplyIndustrialStyle();

    Serial serial("COM3");
    bool serialOk = false;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        serialOk = serial.isConnected();
        if (serialOk) {
            while (serial.available()) {
                std::string line = serial.readLine();
                if (line.empty()) continue;

                if      (line.rfind("T:", 0) == 0) sscanf(line.c_str(), "T: %f", &temperatura);
                else if (line.rfind("N:", 0) == 0) sscanf(line.c_str(), "N: %f", &nivel);
                else if (line.rfind("P:", 0) == 0) sscanf(line.c_str(), "P: %f", &presion);
                else if (line.find("ALERTA") != std::string::npos) {
                    std::string type = "OTRO";
                    if      (line.find("TEMP")  != std::string::npos) type = "TEMP";
                    else if (line.find("NIVEL") != std::string::npos) type = "NIVEL";
                    else if (line.find("PRES")  != std::string::npos) type = "PRESION";

                    if ((int)alerts.size() >= MAX_ALERTS) alerts.erase(alerts.begin());
                    alerts.push_back({line, GetTimestamp(), false, type});
                    unacknowledgedCount++;
                }
                else if (line.rfind("OK:", 0) == 0) {
                    lastCommandFeedback = line;
                    lastCommandTime     = time(nullptr);
                }
                else if (line.rfind("TIPO:", 0) == 0) {
                    rawStateText += line + "\n";
                }
            }
        }

        for (int i = 0; i < TEMP_HISTORY - 1; i++)
            tempHistory[i] = tempHistory[i + 1];
        tempHistory[TEMP_HISTORY - 1] = temperatura;

        //VENTANA PRINCIPAL
        int fbW, fbH;
        glfwGetFramebufferSize(window, &fbW, &fbH);

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2((float)fbW, (float)fbH));
        ImGui::Begin("##Main", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove     | ImGuiWindowFlags_NoCollapse);

        ImGui::TextColored(Colors::TextAccent, "Sistema de Monitoreo Industrial | ESP32");
        ImGui::SameLine((float)fbW - 240);
        ImGui::TextColored(serialOk ? Colors::Normal : Colors::Danger,
            serialOk ? "COM 3 CONECTADO" : "COM 3 SIN CONEXIÓN");
        ImGui::SameLine();
        ImGui::TextColored(Colors::TextMuted, "%s", GetTimestamp().c_str());

        ImGui::Separator();

        float leftW  = (float)fbW * 0.62f - 20;
        float rightW = (float)fbW * 0.38f;

        //Panel sensores
        ImGui::BeginChild("Sensores", ImVec2(leftW, 440), true);
        ImGui::TextColored(Colors::TextAccent, "SENSORES EN TIEMPO REAL");

        {
            bool   enPeligro = (temperatura > TEMP_DANGER);
            ImVec4 textColor = enPeligro ? Colors::Danger : Colors::Normal;

            ImGui::Text("Temperatura: ");
            ImGui::SameLine();
            ImGui::TextColored(textColor, "%.1f °C ", temperatura);
            ImGui::SameLine();
            ImGui::TextColored(textColor, "[%s]", enPeligro ? "PELIGRO" : "NORMAL");

            ImVec4 plotColor = enPeligro ? Colors::Danger : ImVec4(0.0f, 0.85f, 0.6f, 1.0f);
            ImGui::PushStyleColor(ImGuiCol_PlotLines, plotColor);
            ImGui::PushStyleColor(ImGuiCol_FrameBg,   ImVec4(0.06f,0.08f,0.10f,1.f));
            ImGui::PlotLines("##tempPlot", tempHistory, TEMP_HISTORY, 0, "",
                             0.0f, TEMP_MAX, ImVec2(-1, 180));
            ImGui::PopStyleColor(2);
        }

        ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

        DrawSmartBar("NIVEL DE TANQUE", nivel,   100.f, 0,            0,             "%",   true);
        DrawSmartBar("PRESION",         presion, 100.f, PRESION_WARN, PRESION_DANGER, " PSI", false);

        ImGui::EndChild();

        //Panel de control
        ImGui::SameLine();
        ImGui::BeginChild("Control", ImVec2(rightW, 440), true);
        ImGui::TextColored(Colors::TextAccent, "PANEL DE CONTROL");

        float btnW = ImGui::GetContentRegionAvail().x - 10;

        if (StyledButton("  Consultar Estados",
                         Colors::BtnQuery, Colors::BtnQueryHov, Colors::BtnQueryAct, btnW)) {
            serial.write("GET:ESTADO\n");
            rawStateText        = "";
            showStateWindow     = true;
            lastCommandFeedback = "Consultando estados...";
            lastCommandTime     = time(nullptr);
        }

        ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
        ImGui::TextColored(Colors::TextMuted, "RESETEAR ALARMAS");
        ImGui::Spacing();

        if (StyledButton("  Resetear Temperatura",
                         Colors::BtnNormal, Colors::BtnHover, Colors::BtnActive, btnW)) {
            serial.write("CLEAR:TEMP\n");
            alerts.erase(std::remove_if(alerts.begin(), alerts.end(),
                [](const AlertEntry& a){ return a.type == "TEMP"; }), alerts.end());
        }
        if (StyledButton("  Resetear Nivel",
                         Colors::BtnNormal, Colors::BtnHover, Colors::BtnActive, btnW)) {
            serial.write("CLEAR:NIVEL\n");
            alerts.erase(std::remove_if(alerts.begin(), alerts.end(),
                [](const AlertEntry& a){ return a.type == "NIVEL"; }), alerts.end());
        }
        if (StyledButton("  Resetear Presión",
                         Colors::BtnNormal, Colors::BtnHover, Colors::BtnActive, btnW)) {
            serial.write("CLEAR:PRESION\n");
            alerts.erase(std::remove_if(alerts.begin(), alerts.end(),
                [](const AlertEntry& a){ return a.type == "PRESION"; }), alerts.end());
        }
        if (StyledButton("  Resetear TODAS las Alarmas",
                         Colors::BtnDanger, Colors::BtnDangerHov, Colors::BtnDangerAct, btnW)) {
            serial.write("CLEAR:TODO\n");
            alerts.clear();
            unacknowledgedCount = 0;
        }

        if (!lastCommandFeedback.empty() && (time(nullptr) - lastCommandTime < 5))
            ImGui::TextColored(Colors::Normal, "-> %s", lastCommandFeedback.c_str());

        ImGui::EndChild();

        //Historial de alertas
        ImGui::BeginChild("Alertas", ImVec2(-1, -1), true);
        ImGui::TextColored(Colors::Danger, "HISTORIAL DE ALERTAS (%d Alarmas Activas)", unacknowledgedCount);
        ImGui::Separator();
        if (ImGui::BeginTable("alerts", 2, ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("Hora",    ImGuiTableColumnFlags_WidthFixed,   80);
            ImGui::TableSetupColumn("Mensaje", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();
            for (int i = (int)alerts.size()-1; i >= 0; i--) {
                const auto& a = alerts[i];
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("%s", a.timestamp.c_str());
                ImGui::TableSetColumnIndex(1); ImGui::TextColored(Colors::Danger, "%s", a.message.c_str());
            }
            ImGui::EndTable();
        }
        ImGui::EndChild();

        ImGui::End();

        //VENTANA ESTADO
        if (showStateWindow) {
            ImGui::OpenPopup("Estado Actual de las Alarmas");
            if (ImGui::BeginPopupModal("Estado Actual de las Alarmas", &showStateWindow,
                                       ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::SetNextWindowSize(ImVec2(660, 0));

                ImGui::TextColored(Colors::TextAccent, "ESTADO ACTUAL DE LAS ALARMAS");
                ImGui::Separator();
                ImGui::Spacing();

                std::istringstream iss(rawStateText);
                std::string line;
                bool first = true;
                while (std::getline(iss, line)) {
                    if (!line.empty() && line.find("TIPO:") != std::string::npos) {
                        if (!first) {
                            ImGui::Spacing();
                            ImGui::PushStyleColor(ImGuiCol_Separator, Colors::PanelBorder);
                            ImGui::Separator();
                            ImGui::PopStyleColor();
                            ImGui::Spacing();
                        }
                        first = false;
                        DisplayAlarmState(line);
                    }
                }

                if (first) {
                    // No llegó ningún dato todavía
                    ImGui::TextColored(Colors::TextMuted, "Esperando respuesta del ESP32...");
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                if (StyledButton("  Cerrar", Colors::BtnNormal, Colors::BtnHover, Colors::BtnActive, 180.f)) {
                    showStateWindow = false;
                    rawStateText    = "";
                }
                ImGui::EndPopup();
            }
        }

        //Render
        ImGui::Render();
        int dw, dh;
        glfwGetFramebufferSize(window, &dw, &dh);
        glViewport(0, 0, dw, dh);
        glClearColor(0.08f, 0.09f, 0.11f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();
    return 0;
}