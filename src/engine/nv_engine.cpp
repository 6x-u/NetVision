/*
 * NetVision - nv_engine.cpp
 * Developer: MERO:TG@QP4RM
 */

#include "nv_engine.hpp"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <GLFW/glfw3.h>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <algorithm>

namespace nv {

static void glfw_error_cb(int error, const char *desc)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, desc);
}

Engine::Engine()
    : m_window(nullptr)
    , m_capture(nullptr)
    , m_graph(nullptr)
    , m_sessions(nullptr)
    , m_replay(nullptr)
    , m_width(1280)
    , m_height(720)
    , m_zoom(1.0f)
    , m_pan_x(0.0f)
    , m_pan_y(0.0f)
{
    memset(&m_state, 0, sizeof(m_state));
    m_state.running = true;
    m_state.time_scale = 1.0f;
    m_state.show_stats = true;
    m_state.filter_port = -1;
    m_state.filter_protocol = -1;
    m_state.selected_node = -1;
    m_state.selected_packet = -1;
    m_state.selected_session = -1;
}

Engine::~Engine()
{
    shutdown();
}

int Engine::init(int width, int height, const char *title)
{
    glfwSetErrorCallback(glfw_error_cb);
    if (!glfwInit())
        return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    GLFWmonitor *monitor = nullptr;
    const char *fs_env = getenv("NV_FULLSCREEN");
    if (fs_env && fs_env[0] == '1') {
        monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode *mode = glfwGetVideoMode(monitor);
        width = mode->width;
        height = mode->height;
        glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    }

    m_width = width;
    m_height = height;
    m_window = glfwCreateWindow(width, height, title, monitor, nullptr);
    if (!m_window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();
    ImGuiStyle &style = ImGui::GetStyle();
    style.WindowRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.08f, 0.12f, 0.95f);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.10f, 0.18f, 1.00f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.15f, 0.15f, 0.28f, 1.00f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.20f, 0.25f, 0.45f, 1.00f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.28f, 0.35f, 0.60f, 1.00f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.20f, 0.25f, 0.45f, 0.80f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.28f, 0.35f, 0.60f, 0.80f);

    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    m_graph = nv_graph_create();
    m_sessions = nv_session_mgr_create();
    m_replay = nv_replay_create(500000);

    m_state.status_msg = "NetVision Ready - MERO:TG@QP4RM";
    return 0;
}

void Engine::run()
{
    double last_time = glfwGetTime();

    while (m_state.running && !glfwWindowShouldClose(m_window)) {
        double now = glfwGetTime();
        float dt = (float)(now - last_time);
        last_time = now;

        glfwPollEvents();
        handle_input();

        if (m_state.capturing && !m_state.time_freeze)
            process_packets();

        if (!m_state.time_freeze)
            update_animations(dt);

        nv_graph_update_layout(m_graph, (float)m_width, (float)m_height);
        nv_session_cleanup_idle(m_sessions, 30.0);

        render_frame();

        glfwSwapBuffers(m_window);
    }
}

void Engine::shutdown()
{
    if (m_state.capturing)
        stop_capture();

    if (m_window) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        glfwDestroyWindow(m_window);
        glfwTerminate();
        m_window = nullptr;
    }

    if (m_graph)    { nv_graph_destroy(m_graph);         m_graph = nullptr; }
    if (m_sessions) { nv_session_mgr_destroy(m_sessions); m_sessions = nullptr; }
    if (m_replay)   { nv_replay_destroy(m_replay);       m_replay = nullptr; }
    if (m_capture)  { nv_capture_destroy(m_capture);     m_capture = nullptr; }
}

void Engine::process_packets()
{
    nv_packet_t pkt;
    int processed = 0;

    while (processed < 256 && nv_capture_next_packet(m_capture, &pkt) == 0) {
        nv_graph_process_packet(m_graph, &pkt);
        nv_session_track(m_sessions, &pkt);

        if (m_state.recording)
            nv_replay_record_packet(m_replay, &pkt);

        nv_node_t *src = nv_graph_get_node(m_graph, pkt.src_node_id);
        nv_node_t *dst = nv_graph_get_node(m_graph, pkt.dst_node_id);

        if (src && dst) {
            AnimPacket ap;
            ap.data = pkt;
            ap.progress = 0.0f;
            ap.speed = (pkt.latency_ms > 0) ? (1.0f / (float)(pkt.latency_ms / 100.0)) : 2.0f;
            if (ap.speed < 0.2f) ap.speed = 0.2f;
            if (ap.speed > 5.0f) ap.speed = 5.0f;
            ap.alive = true;

            std::lock_guard<std::mutex> lock(m_mutex);
            m_anim_packets.push_back(ap);
        }
        processed++;
    }
}

void Engine::update_animations(float dt)
{
    float scale = m_state.slow_motion ? 0.1f : m_state.time_scale;
    std::lock_guard<std::mutex> lock(m_mutex);

    for (auto &ap : m_anim_packets) {
        if (!ap.alive)
            continue;
        ap.progress += ap.speed * dt * scale;
        if (ap.progress >= 1.0f)
            ap.alive = false;
    }

    m_anim_packets.erase(
        std::remove_if(m_anim_packets.begin(), m_anim_packets.end(),
                       [](const AnimPacket &a) { return !a.alive; }),
        m_anim_packets.end());
}

void Engine::render_frame()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    glfwGetFramebufferSize(m_window, &m_width, &m_height);
    glViewport(0, 0, m_width, m_height);
    glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    render_menu_bar();
    render_toolbar();
    render_network_view();

    if (m_state.show_stats)    render_stats_window();
    if (m_state.show_filters)  render_filter_window();
    if (m_state.show_sessions) render_session_window();
    if (m_state.show_replay)   render_replay_window();
    if (m_state.show_details)  render_detail_window();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Engine::render_network_view()
{
    ImGui::SetNextWindowPos(ImVec2(0, 60));
    ImGui::SetNextWindowSize(ImVec2((float)m_width, (float)m_height - 80));
    ImGui::Begin("##NetworkView", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground |
                 ImGuiWindowFlags_NoBringToFrontOnFocus);

    uint32_t edge_count = nv_graph_edge_count(m_graph);
    nv_edge_t *edges = nv_graph_get_edges(m_graph);
    for (uint32_t i = 0; i < edge_count; i++)
        render_edge(&edges[i]);

    render_animated_packets();

    uint32_t node_count = nv_graph_node_count(m_graph);
    nv_node_t *nodes = nv_graph_get_nodes(m_graph);
    for (uint32_t i = 0; i < node_count; i++) {
        if (!nodes[i].active)
            continue;
        bool selected = ((int)nodes[i].id == m_state.selected_node);
        render_node(&nodes[i], selected);
    }

    ImGui::End();
}

void Engine::render_node(const nv_node_t *node, bool selected)
{
    ImDrawList *dl = ImGui::GetWindowDrawList();
    ImVec2 pos(node->pos_x + m_pan_x, node->pos_y + m_pan_y);
    float radius = 20.0f * m_zoom;
    ImU32 color;

    switch (node->type) {
    case NV_NODE_CLIENT:  color = IM_COL32(0, 200, 0, 255);   break;
    case NV_NODE_SERVER:  color = IM_COL32(0, 120, 255, 255);  break;
    case NV_NODE_API:     color = IM_COL32(180, 0, 255, 255);  break;
    case NV_NODE_GATEWAY: color = IM_COL32(255, 165, 0, 255);  break;
    default:              color = IM_COL32(128, 128, 128, 255); break;
    }

    if (selected) {
        dl->AddCircleFilled(pos, radius + 4.0f, IM_COL32(255, 255, 255, 100), 32);
    }

    dl->AddCircleFilled(pos, radius, color, 32);
    dl->AddCircle(pos, radius, IM_COL32(255, 255, 255, 80), 32, 2.0f);

    char label[64];
    snprintf(label, sizeof(label), "%s", node->ip);
    ImVec2 text_size = ImGui::CalcTextSize(label);
    dl->AddText(ImVec2(pos.x - text_size.x * 0.5f, pos.y + radius + 4.0f),
                IM_COL32(200, 200, 200, 255), label);

    snprintf(label, sizeof(label), "[%s]", node_type_name(node->type));
    text_size = ImGui::CalcTextSize(label);
    dl->AddText(ImVec2(pos.x - text_size.x * 0.5f, pos.y + radius + 18.0f),
                IM_COL32(150, 150, 150, 200), label);

    ImVec2 mouse = ImGui::GetMousePos();
    float dx = mouse.x - pos.x;
    float dy = mouse.y - pos.y;
    if (dx * dx + dy * dy < radius * radius) {
        if (ImGui::IsMouseClicked(0)) {
            m_state.selected_node = node->id;
            m_state.show_details = true;
        }
        ImGui::SetTooltip("%s\nType: %s\nPort: %u\nSent: %lu B\nRecv: %lu B\nPackets: %u",
                          node->ip, node_type_name(node->type), node->port,
                          (unsigned long)node->bytes_sent,
                          (unsigned long)node->bytes_recv,
                          node->pkt_count);
    }
}

void Engine::render_edge(const nv_edge_t *edge)
{
    ImDrawList *dl = ImGui::GetWindowDrawList();
    nv_node_t *src = nv_graph_get_node(m_graph, edge->src_node_id);
    nv_node_t *dst = nv_graph_get_node(m_graph, edge->dst_node_id);

    if (!src || !dst || !src->active || !dst->active)
        return;

    ImVec2 p1(src->pos_x + m_pan_x, src->pos_y + m_pan_y);
    ImVec2 p2(dst->pos_x + m_pan_x, dst->pos_y + m_pan_y);

    float thickness = 1.0f + std::min((float)edge->pkt_count / 50.0f, 4.0f);
    ImU32 color;

    if (edge->congested)
        color = IM_COL32(255, 60, 60, 180);
    else if (edge->avg_latency_ms > 100.0)
        color = IM_COL32(255, 200, 0, 150);
    else
        color = IM_COL32(100, 150, 200, 100);

    dl->AddLine(p1, p2, color, thickness);
}

void Engine::render_animated_packets()
{
    ImDrawList *dl = ImGui::GetWindowDrawList();
    std::lock_guard<std::mutex> lock(m_mutex);

    for (const auto &ap : m_anim_packets) {
        nv_node_t *src = nv_graph_get_node(m_graph, ap.data.src_node_id);
        nv_node_t *dst = nv_graph_get_node(m_graph, ap.data.dst_node_id);
        if (!src || !dst)
            continue;

        float t = ap.progress;
        float x = src->pos_x + (dst->pos_x - src->pos_x) * t + m_pan_x;
        float y = src->pos_y + (dst->pos_y - src->pos_y) * t + m_pan_y;

        ImU32 color = packet_color(ap.data.status);
        float radius = 3.0f + (float)ap.data.size / 5000.0f;
        if (radius > 8.0f) radius = 8.0f;
        radius *= m_zoom;

        dl->AddCircleFilled(ImVec2(x, y), radius, color, 12);
        dl->AddCircle(ImVec2(x, y), radius + 1.0f, IM_COL32(255, 255, 255, 60), 12);
    }
}

void Engine::render_menu_bar()
{
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Reset Graph"))
                nv_graph_reset(m_graph);
            ImGui::Separator();
            if (ImGui::MenuItem("Exit"))
                m_state.running = false;
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Statistics", nullptr, &m_state.show_stats);
            ImGui::MenuItem("Filters", nullptr, &m_state.show_filters);
            ImGui::MenuItem("Sessions", nullptr, &m_state.show_sessions);
            ImGui::MenuItem("Replay", nullptr, &m_state.show_replay);
            ImGui::MenuItem("Details", nullptr, &m_state.show_details);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Capture")) {
            if (!m_state.capturing) {
                if (ImGui::MenuItem("Start Capture"))
                    start_capture();
            } else {
                if (ImGui::MenuItem("Stop Capture"))
                    stop_capture();
            }
            ImGui::EndMenu();
        }

        ImGui::SameLine((float)m_width - 350.0f);
        ImGui::Text("| %s", m_state.status_msg.c_str());

        ImGui::EndMainMenuBar();
    }
}

void Engine::render_toolbar()
{
    ImGui::SetNextWindowPos(ImVec2(0, 19));
    ImGui::SetNextWindowSize(ImVec2((float)m_width, 40));
    ImGui::Begin("##Toolbar", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);

    if (!m_state.capturing) {
        if (ImGui::Button("[ START ]"))
            start_capture();
    } else {
        if (ImGui::Button("[ STOP ]"))
            stop_capture();
    }

    ImGui::SameLine();
    if (ImGui::Button(m_state.paused ? "RESUME" : "PAUSE"))
        toggle_pause();

    ImGui::SameLine();
    if (ImGui::Button(m_state.slow_motion ? "NORMAL SPEED" : "SLOW MOTION"))
        toggle_slow_motion();

    ImGui::SameLine();
    if (ImGui::Button(m_state.time_freeze ? "UNFREEZE" : "TIME FREEZE"))
        toggle_time_freeze();

    ImGui::SameLine();
    ImGui::Text("  Nodes: %u  Edges: %u  Packets: %zu",
                nv_graph_node_count(m_graph),
                nv_graph_edge_count(m_graph),
                m_anim_packets.size());

    ImGui::SameLine();
    ImGui::SetNextItemWidth(120);
    ImGui::SliderFloat("Zoom", &m_zoom, 0.1f, 5.0f, "%.1fx");

    ImGui::End();
}

void Engine::render_stats_window()
{
    ImGui::SetNextWindowPos(ImVec2(10, 70), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(280, 240), ImGuiCond_FirstUseEver);
    ImGui::Begin("Network Statistics", &m_state.show_stats);

    nv_stats_t stats = nv_graph_get_stats(m_graph);

    ImGui::Text("Active Connections:  %u", stats.active_connections);
    ImGui::Text("Total Errors:        %u", stats.error_count);
    ImGui::Text("Retransmissions:     %u", stats.retransmit_count);
    ImGui::Text("Avg Latency:         %.2f ms", stats.avg_latency_ms);
    ImGui::Text("Packet Loss Rate:    %.4f%%", stats.packet_loss_rate * 100.0);
    ImGui::Separator();
    ImGui::Text("Total Nodes:         %u", nv_graph_node_count(m_graph));
    ImGui::Text("Total Edges:         %u", nv_graph_edge_count(m_graph));
    ImGui::Text("Active Sessions:     %u", nv_session_active_count(m_sessions));
    ImGui::Text("Total Sessions:      %u", nv_session_count(m_sessions));

    ImGui::End();
}

void Engine::render_filter_window()
{
    ImGui::SetNextWindowPos(ImVec2(10, 320), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(280, 220), ImGuiCond_FirstUseEver);
    ImGui::Begin("Filters", &m_state.show_filters);

    ImGui::InputText("IP Address", m_state.filter_ip, NV_MAX_IP_LEN);
    ImGui::InputInt("Port", &m_state.filter_port);

    const char *protos[] = { "All", "TCP", "UDP", "HTTP", "HTTPS", "DNS", "ICMP" };
    ImGui::Combo("Protocol", &m_state.filter_protocol, protos, 7);
    ImGui::Checkbox("Errors Only", &m_state.filter_errors_only);

    if (ImGui::Button("Apply Filters"))
        apply_filters();
    ImGui::SameLine();
    if (ImGui::Button("Clear")) {
        memset(m_state.filter_ip, 0, NV_MAX_IP_LEN);
        m_state.filter_port = -1;
        m_state.filter_protocol = -1;
        m_state.filter_errors_only = false;
    }

    ImGui::End();
}

void Engine::render_session_window()
{
    ImGui::SetNextWindowPos(ImVec2((float)m_width - 450, 70), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(440, 350), ImGuiCond_FirstUseEver);
    ImGui::Begin("Sessions", &m_state.show_sessions);

    uint32_t count = nv_session_count(m_sessions);
    nv_session_t *all = nv_session_get_all(m_sessions);

    if (ImGui::BeginTable("##SessTable", 6,
            ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY)) {
        ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 40);
        ImGui::TableSetupColumn("Protocol", ImGuiTableColumnFlags_WidthFixed, 60);
        ImGui::TableSetupColumn("State", ImGuiTableColumnFlags_WidthFixed, 60);
        ImGui::TableSetupColumn("Packets", ImGuiTableColumnFlags_WidthFixed, 60);
        ImGui::TableSetupColumn("Bytes", ImGuiTableColumnFlags_WidthFixed, 80);
        ImGui::TableSetupColumn("Errors", ImGuiTableColumnFlags_WidthFixed, 50);
        ImGui::TableHeadersRow();

        for (uint32_t i = 0; i < count && i < 200; i++) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            char sel_label[16];
            snprintf(sel_label, sizeof(sel_label), "%u", all[i].id);
            if (ImGui::Selectable(sel_label, m_state.selected_session == (int)all[i].id,
                                  ImGuiSelectableFlags_SpanAllColumns)) {
                m_state.selected_session = all[i].id;
                m_state.show_details = true;
            }
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", protocol_name(all[i].protocol));
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%s", session_state_name(all[i].state));
            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%u", all[i].pkt_count);
            ImGui::TableSetColumnIndex(4);
            ImGui::Text("%lu", (unsigned long)all[i].bytes_transferred);
            ImGui::TableSetColumnIndex(5);
            ImGui::Text("%u", all[i].errors);
        }
        ImGui::EndTable();
    }

    ImGui::End();
}

void Engine::render_replay_window()
{
    ImGui::SetNextWindowPos(ImVec2(300, (float)m_height - 180), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(400, 160), ImGuiCond_FirstUseEver);
    ImGui::Begin("Replay Control", &m_state.show_replay);

    if (!nv_replay_is_recording(m_replay)) {
        if (ImGui::Button("Start Recording")) {
            nv_replay_start_recording(m_replay);
            m_state.recording = true;
            m_state.status_msg = "Recording...";
        }
    } else {
        if (ImGui::Button("Stop Recording")) {
            nv_replay_stop_recording(m_replay);
            m_state.recording = false;
            m_state.status_msg = "Recording stopped";
        }
    }

    ImGui::SameLine();
    if (!nv_replay_is_playing(m_replay)) {
        if (ImGui::Button("Play")) {
            nv_replay_start_playback(m_replay, 1.0);
            m_state.status_msg = "Playing replay...";
        }
    } else {
        if (ImGui::Button("Stop Playback")) {
            nv_replay_stop_playback(m_replay);
            m_state.status_msg = "Playback stopped";
        }
    }

    ImGui::Text("Frames: %u", nv_replay_get_frame_count(m_replay));
    ImGui::Text("Duration: %.2f sec", nv_replay_get_duration(m_replay));

    static float replay_speed = 1.0f;
    if (ImGui::SliderFloat("Playback Speed", &replay_speed, 0.1f, 10.0f, "%.1fx")) {
        nv_replay_set_speed(m_replay, replay_speed);
    }

    static char save_path[256] = "capture.nvrp";
    ImGui::InputText("File", save_path, sizeof(save_path));
    ImGui::SameLine();
    if (ImGui::Button("Save"))
        nv_replay_save(m_replay, save_path);
    ImGui::SameLine();
    if (ImGui::Button("Load"))
        nv_replay_load(m_replay, save_path);

    ImGui::End();
}

void Engine::render_detail_window()
{
    ImGui::SetNextWindowPos(ImVec2((float)m_width - 350, (float)m_height - 300), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(340, 280), ImGuiCond_FirstUseEver);
    ImGui::Begin("Details", &m_state.show_details);

    if (m_state.selected_node > 0) {
        nv_node_t *node = nv_graph_get_node(m_graph, m_state.selected_node);
        if (node) {
            ImGui::Text("Node Details");
            ImGui::Separator();
            ImGui::Text("IP:       %s", node->ip);
            ImGui::Text("Type:     %s", node_type_name(node->type));
            ImGui::Text("Port:     %u", node->port);
            ImGui::Text("Sent:     %lu bytes", (unsigned long)node->bytes_sent);
            ImGui::Text("Received: %lu bytes", (unsigned long)node->bytes_recv);
            ImGui::Text("Packets:  %u", node->pkt_count);
            ImGui::Text("Active:   %s", node->active ? "Yes" : "No");
        }
    }

    if (m_state.selected_session > 0) {
        nv_session_t *sess = nv_session_get(m_sessions, m_state.selected_session);
        if (sess) {
            ImGui::Separator();
            ImGui::Text("Session Details");
            ImGui::Separator();
            ImGui::Text("ID:           %u", sess->id);
            ImGui::Text("Protocol:     %s", protocol_name(sess->protocol));
            ImGui::Text("State:        %s", session_state_name(sess->state));
            ImGui::Text("Packets:      %u", sess->pkt_count);
            ImGui::Text("Bytes:        %lu", (unsigned long)sess->bytes_transferred);
            ImGui::Text("Avg Latency:  %.2f ms",
                        sess->pkt_count > 0 ? sess->total_latency_ms / sess->pkt_count : 0.0);
            ImGui::Text("Retransmits:  %u", sess->retransmits);
            ImGui::Text("Errors:       %u", sess->errors);
        }
    }

    ImGui::End();
}

void Engine::handle_input()
{
    ImGuiIO &io = ImGui::GetIO();
    if (io.WantCaptureMouse)
        return;

    if (ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
        ImVec2 delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Right);
        m_pan_x += delta.x;
        m_pan_y += delta.y;
        ImGui::ResetMouseDragDelta(ImGuiMouseButton_Right);
    }

    float wheel = io.MouseWheel;
    if (wheel != 0.0f) {
        m_zoom += wheel * 0.1f;
        if (m_zoom < 0.1f) m_zoom = 0.1f;
        if (m_zoom > 5.0f) m_zoom = 5.0f;
    }
}

void Engine::start_capture()
{
    if (m_state.capturing)
        return;

    m_capture = nv_capture_create(
        m_state.capture_device.empty() ? nullptr : m_state.capture_device.c_str(),
        65535, 1);

    if (!m_capture) {
        m_state.status_msg = "Failed to create capture context";
        return;
    }

    if (nv_capture_start(m_capture) != 0) {
        m_state.status_msg = std::string("Capture error: ") + nv_capture_get_error(m_capture);
        nv_capture_destroy(m_capture);
        m_capture = nullptr;
        return;
    }

    m_state.capturing = true;
    m_state.status_msg = "Capturing...";
}

void Engine::stop_capture()
{
    if (!m_state.capturing || !m_capture)
        return;

    nv_capture_stop(m_capture);
    nv_capture_destroy(m_capture);
    m_capture = nullptr;
    m_state.capturing = false;
    m_state.status_msg = "Capture stopped";
}

void Engine::toggle_pause()
{
    m_state.paused = !m_state.paused;
    m_state.time_scale = m_state.paused ? 0.0f : 1.0f;
}

void Engine::toggle_slow_motion()
{
    m_state.slow_motion = !m_state.slow_motion;
}

void Engine::toggle_time_freeze()
{
    m_state.time_freeze = !m_state.time_freeze;
    m_state.status_msg = m_state.time_freeze ? "TIME FROZEN" : "Time resumed";
}

void Engine::apply_filters()
{
    nv_filter_t filter;
    memset(&filter, 0, sizeof(filter));
    filter.enabled = 1;
    strncpy(filter.src_ip, m_state.filter_ip, NV_MAX_IP_LEN - 1);
    filter.port = (m_state.filter_port > 0) ? (uint16_t)m_state.filter_port : 0;
    if (m_state.filter_protocol > 0)
        filter.protocol = (nv_protocol_t)m_state.filter_protocol;
    filter.show_errors_only = m_state.filter_errors_only ? 1 : 0;
    nv_graph_apply_filter(m_graph, &filter);
}

const char *Engine::protocol_name(nv_protocol_t proto)
{
    switch (proto) {
    case NV_PROTO_TCP:       return "TCP";
    case NV_PROTO_UDP:       return "UDP";
    case NV_PROTO_HTTP:      return "HTTP";
    case NV_PROTO_HTTPS:     return "HTTPS";
    case NV_PROTO_DNS:       return "DNS";
    case NV_PROTO_ICMP:      return "ICMP";
    case NV_PROTO_ARP:       return "ARP";
    case NV_PROTO_WEBSOCKET: return "WS";
    default:                 return "UNKNOWN";
    }
}

const char *Engine::node_type_name(nv_node_type_t type)
{
    switch (type) {
    case NV_NODE_CLIENT:  return "Client";
    case NV_NODE_SERVER:  return "Server";
    case NV_NODE_API:     return "API";
    case NV_NODE_GATEWAY: return "Gateway";
    default:              return "Unknown";
    }
}

const char *Engine::session_state_name(nv_session_state_t state)
{
    switch (state) {
    case NV_SESS_ACTIVE: return "Active";
    case NV_SESS_IDLE:   return "Idle";
    case NV_SESS_CLOSED: return "Closed";
    case NV_SESS_ERROR:  return "Error";
    default:             return "Unknown";
    }
}

uint32_t Engine::packet_color(nv_packet_status_t status)
{
    switch (status) {
    case NV_PKT_NORMAL:      return IM_COL32(0, 220, 0, 230);
    case NV_PKT_SLOW:        return IM_COL32(255, 220, 0, 230);
    case NV_PKT_ERROR:       return IM_COL32(255, 40, 40, 255);
    case NV_PKT_RETRANSMIT:  return IM_COL32(255, 140, 0, 230);
    case NV_PKT_TIMEOUT:     return IM_COL32(200, 0, 0, 255);
    default:                 return IM_COL32(180, 180, 180, 200);
    }
}

}
