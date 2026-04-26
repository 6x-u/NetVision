/*
 * NetVision - nv_engine.hpp
 * Main rendering engine using ImGui + OpenGL/GLFW.
 * Developer: MERO:TG@QP4RM
 */

#ifndef NV_ENGINE_HPP
#define NV_ENGINE_HPP

#include "nv_types.h"
#include "nv_capture.h"
#include "nv_graph.h"
#include "nv_session.h"
#include "nv_replay.h"

#include <vector>
#include <string>
#include <mutex>
#include <atomic>

struct GLFWwindow;

namespace nv {

struct AnimPacket {
    nv_packet_t     data;
    float           progress;
    float           speed;
    bool            alive;
};

struct AppState {
    bool            running;
    bool            capturing;
    bool            paused;
    bool            slow_motion;
    bool            time_freeze;
    float           time_scale;
    int             selected_node;
    int             selected_packet;
    int             selected_session;
    bool            show_stats;
    bool            show_filters;
    bool            show_sessions;
    bool            show_replay;
    bool            show_details;
    bool            recording;
    char            filter_ip[NV_MAX_IP_LEN];
    int             filter_port;
    int             filter_protocol;
    bool            filter_errors_only;
    std::string     capture_device;
    std::string     status_msg;
};

class Engine {
public:
    Engine();
    ~Engine();

    int  init(int width, int height, const char *title);
    void run();
    void shutdown();

private:
    void process_packets();
    void update_animations(float dt);
    void render_frame();
    void render_network_view();
    void render_node(const nv_node_t *node, bool selected);
    void render_edge(const nv_edge_t *edge);
    void render_animated_packets();
    void render_menu_bar();
    void render_stats_window();
    void render_filter_window();
    void render_session_window();
    void render_replay_window();
    void render_detail_window();
    void render_toolbar();
    void handle_input();

    void start_capture();
    void stop_capture();
    void toggle_pause();
    void toggle_slow_motion();
    void toggle_time_freeze();
    void apply_filters();

    const char *protocol_name(nv_protocol_t proto);
    const char *node_type_name(nv_node_type_t type);
    const char *session_state_name(nv_session_state_t state);
    uint32_t    packet_color(nv_packet_status_t status);

    ::GLFWwindow         *m_window;
    nv_capture_ctx_t     *m_capture;
    nv_graph_t           *m_graph;
    nv_session_mgr_t     *m_sessions;
    nv_replay_t          *m_replay;

    std::vector<AnimPacket> m_anim_packets;
    std::mutex              m_mutex;
    AppState                m_state;

    int   m_width;
    int   m_height;
    float m_zoom;
    float m_pan_x;
    float m_pan_y;
};

}

#endif
