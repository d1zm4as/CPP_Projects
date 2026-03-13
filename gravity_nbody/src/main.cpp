#include <SFML/Graphics.hpp>
#include <cmath>
#include <deque>
#include <fstream>
#include <iomanip>
#include <random>
#include <sstream>
#include <string>
#include <vector>

struct Vec2 {
    float x;
    float y;

    Vec2 operator+(const Vec2& o) const { return {x + o.x, y + o.y}; }
    Vec2 operator-(const Vec2& o) const { return {x - o.x, y - o.y}; }
    Vec2 operator*(float s) const { return {x * s, y * s}; }
    Vec2& operator+=(const Vec2& o) { x += o.x; y += o.y; return *this; }
    Vec2& operator-=(const Vec2& o) { x -= o.x; y -= o.y; return *this; }
    Vec2& operator*=(float s) { x *= s; y *= s; return *this; }
};

struct Body {
    Vec2 pos;
    Vec2 vel;
    float mass;
};

enum class Preset {
    Galaxy,
    Ring,
    Swarm
};

static float length(const Vec2& v) {
    return std::sqrt(v.x * v.x + v.y * v.y);
}

static sf::Color speed_color(float speed) {
    float t = std::min(speed / 200.0f, 1.0f);
    sf::Uint8 r = static_cast<sf::Uint8>(255.0f * t);
    sf::Uint8 g = static_cast<sf::Uint8>(200.0f * (1.0f - std::abs(t - 0.5f) * 2.0f));
    sf::Uint8 b = static_cast<sf::Uint8>(255.0f * (1.0f - t));
    return sf::Color(r, g, b);
}

static const unsigned char* glyph_for(char c) {
    static const unsigned char empty[7] = {0, 0, 0, 0, 0, 0, 0};

    if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');

    static const unsigned char digits[10][7] = {
        {0x0E,0x11,0x13,0x15,0x19,0x11,0x0E},
        {0x04,0x0C,0x04,0x04,0x04,0x04,0x0E},
        {0x0E,0x11,0x01,0x02,0x04,0x08,0x1F},
        {0x1E,0x01,0x01,0x0E,0x01,0x01,0x1E},
        {0x02,0x06,0x0A,0x12,0x1F,0x02,0x02},
        {0x1F,0x10,0x10,0x1E,0x01,0x01,0x1E},
        {0x0E,0x10,0x10,0x1E,0x11,0x11,0x0E},
        {0x1F,0x01,0x02,0x04,0x08,0x08,0x08},
        {0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E},
        {0x0E,0x11,0x11,0x0F,0x01,0x01,0x0E}
    };

    static const unsigned char letters[26][7] = {
        {0x0E,0x11,0x11,0x1F,0x11,0x11,0x11},
        {0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E},
        {0x0E,0x11,0x10,0x10,0x10,0x11,0x0E},
        {0x1E,0x11,0x11,0x11,0x11,0x11,0x1E},
        {0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F},
        {0x1F,0x10,0x10,0x1E,0x10,0x10,0x10},
        {0x0E,0x11,0x10,0x17,0x11,0x11,0x0F},
        {0x11,0x11,0x11,0x1F,0x11,0x11,0x11},
        {0x0E,0x04,0x04,0x04,0x04,0x04,0x0E},
        {0x07,0x02,0x02,0x02,0x12,0x12,0x0C},
        {0x11,0x12,0x14,0x18,0x14,0x12,0x11},
        {0x10,0x10,0x10,0x10,0x10,0x10,0x1F},
        {0x11,0x1B,0x15,0x15,0x11,0x11,0x11},
        {0x11,0x19,0x15,0x13,0x11,0x11,0x11},
        {0x0E,0x11,0x11,0x11,0x11,0x11,0x0E},
        {0x1E,0x11,0x11,0x1E,0x10,0x10,0x10},
        {0x0E,0x11,0x11,0x11,0x15,0x12,0x0D},
        {0x1E,0x11,0x11,0x1E,0x14,0x12,0x11},
        {0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E},
        {0x1F,0x04,0x04,0x04,0x04,0x04,0x04},
        {0x11,0x11,0x11,0x11,0x11,0x11,0x0E},
        {0x11,0x11,0x11,0x11,0x11,0x0A,0x04},
        {0x11,0x11,0x11,0x15,0x15,0x15,0x0A},
        {0x11,0x11,0x0A,0x04,0x0A,0x11,0x11},
        {0x11,0x11,0x0A,0x04,0x04,0x04,0x04},
        {0x1F,0x01,0x02,0x04,0x08,0x10,0x1F}
    };

    switch (c) {
        case ' ': return empty;
        case '.': { static const unsigned char dot[7] = {0,0,0,0,0,0x0C,0x0C}; return dot; }
        case '-': { static const unsigned char dash[7] = {0,0,0,0x1F,0,0,0}; return dash; }
        case ':': { static const unsigned char col[7] = {0,0x0C,0x0C,0,0x0C,0x0C,0}; return col; }
        case '/': { static const unsigned char slash[7] = {0x01,0x02,0x04,0x08,0x10,0x00,0x00}; return slash; }
        default: break;
    }

    if (c >= '0' && c <= '9') return digits[c - '0'];
    if (c >= 'A' && c <= 'Z') return letters[c - 'A'];

    return empty;
}

static void draw_text(sf::RenderWindow& window, float x, float y, float scale, sf::Color color, const std::string& text) {
    sf::RectangleShape pixel({scale, scale});
    pixel.setFillColor(color);

    float cursor_x = x;
    for (char c : text) {
        const unsigned char* glyph = glyph_for(c);
        for (int row = 0; row < 7; ++row) {
            for (int col = 0; col < 5; ++col) {
                if (glyph[row] & (1 << (4 - col))) {
                    pixel.setPosition(cursor_x + col * scale, y + row * scale);
                    window.draw(pixel);
                }
            }
        }
        cursor_x += 6.0f * scale;
    }
}

static std::vector<Vec2> compute_accelerations(const std::vector<Body>& bodies, float G, float softening) {
    std::vector<Vec2> acc(bodies.size(), {0.0f, 0.0f});
    for (size_t i = 0; i < bodies.size(); ++i) {
        for (size_t j = i + 1; j < bodies.size(); ++j) {
            Vec2 r = bodies[j].pos - bodies[i].pos;
            float dist = length(r) + softening;
            float inv = 1.0f / (dist * dist * dist);
            Vec2 force_dir = r * inv;
            Vec2 a_i = force_dir * (G * bodies[j].mass);
            Vec2 a_j = force_dir * (-G * bodies[i].mass);
            acc[i] += a_i;
            acc[j] += a_j;
        }
    }
    return acc;
}

static std::string preset_name(Preset p) {
    switch (p) {
        case Preset::Galaxy: return "GALAXY";
        case Preset::Ring: return "RING";
        case Preset::Swarm: return "SWARM";
        default: return "";
    }
}

static void apply_preset(Preset p, std::vector<Body>& bodies, int body_count, float G,
                         unsigned width, unsigned height, std::mt19937& rng) {
    bodies.clear();
    bodies.reserve(body_count);

    std::uniform_real_distribution<float> massd(2.0f, 10.0f);
    std::uniform_real_distribution<float> jitter(-8.0f, 8.0f);
    std::uniform_real_distribution<float> angle_dist(0.0f, 6.2831853f);

    float cx = width * 0.5f;
    float cy = height * 0.5f;

    if (p == Preset::Galaxy) {
        Body core;
        core.pos = {cx, cy};
        core.vel = {0.0f, 0.0f};
        core.mass = 180.0f;
        bodies.push_back(core);

        std::uniform_real_distribution<float> radius_dist(60.0f, std::min(width, height) * 0.45f);
        std::uniform_real_distribution<float> speed_scale(0.7f, 1.3f);

        for (int i = 1; i < body_count; ++i) {
            float a = angle_dist(rng);
            float r = radius_dist(rng);
            Vec2 pos = {cx + r * std::cos(a), cy + r * std::sin(a)};
            float v = std::sqrt(G * core.mass / std::max(r, 1.0f));
            Vec2 tangent = {-std::sin(a), std::cos(a)};
            Vec2 vel = tangent * (v * speed_scale(rng));
            vel.x += jitter(rng);
            vel.y += jitter(rng);

            Body b;
            b.pos = pos;
            b.vel = vel;
            b.mass = massd(rng);
            bodies.push_back(b);
        }
    } else if (p == Preset::Ring) {
        float r_base = std::min(width, height) * 0.35f;
        std::uniform_real_distribution<float> r_jitter(-20.0f, 20.0f);
        std::uniform_real_distribution<float> v_jitter(-5.0f, 5.0f);

        for (int i = 0; i < body_count; ++i) {
            float a = angle_dist(rng);
            float r = r_base + r_jitter(rng);
            Vec2 pos = {cx + r * std::cos(a), cy + r * std::sin(a)};
            float v = 80.0f + v_jitter(rng);
            Vec2 tangent = {-std::sin(a), std::cos(a)};
            Vec2 vel = tangent * v;

            Body b;
            b.pos = pos;
            b.vel = vel;
            b.mass = massd(rng);
            bodies.push_back(b);
        }
    } else {
        std::uniform_real_distribution<float> posx(100.0f, width - 100.0f);
        std::uniform_real_distribution<float> posy(100.0f, height - 100.0f);
        std::uniform_real_distribution<float> veld(-25.0f, 25.0f);

        for (int i = 0; i < body_count; ++i) {
            Body b;
            b.pos = {posx(rng), posy(rng)};
            b.vel = {veld(rng), veld(rng)};
            b.mass = massd(rng);
            bodies.push_back(b);
        }
    }
}

static bool load_config(const std::string& path, int& body_count, float& G, float& softening,
                        float& dt, bool& show_trails, bool& color_by_speed, size_t& trail_len) {
    std::ifstream in(path);
    if (!in) return false;

    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);

        if (key == "body_count") body_count = std::max(10, std::min(800, std::stoi(val)));
        else if (key == "G") G = std::stof(val);
        else if (key == "softening") softening = std::stof(val);
        else if (key == "dt") dt = std::stof(val);
        else if (key == "show_trails") show_trails = (std::stoi(val) != 0);
        else if (key == "color_by_speed") color_by_speed = (std::stoi(val) != 0);
        else if (key == "trail_len") trail_len = static_cast<size_t>(std::max(0, std::stoi(val)));
    }

    return true;
}

static bool save_config(const std::string& path, int body_count, float G, float softening,
                        float dt, bool show_trails, bool color_by_speed, size_t trail_len) {
    std::ofstream out(path);
    if (!out) return false;

    out << "# gravity_nbody config\n";
    out << "body_count=" << body_count << "\n";
    out << "G=" << G << "\n";
    out << "softening=" << softening << "\n";
    out << "dt=" << dt << "\n";
    out << "show_trails=" << (show_trails ? 1 : 0) << "\n";
    out << "color_by_speed=" << (color_by_speed ? 1 : 0) << "\n";
    out << "trail_len=" << trail_len << "\n";

    return true;
}

static Vec2 center_of_mass(const std::vector<Body>& bodies) {
    Vec2 sum = {0.0f, 0.0f};
    float total = 0.0f;
    for (const auto& b : bodies) {
        sum += b.pos * b.mass;
        total += b.mass;
    }
    if (total <= 0.0f) return {0.0f, 0.0f};
    return sum * (1.0f / total);
}

int main() {
    const unsigned width = 1200;
    const unsigned height = 800;
    const std::string config_path = "config.txt";

    int body_count = 150;
    float G = 200.0f;
    float softening = 8.0f;
    float dt = 0.016f;
    bool show_trails = true;
    bool color_by_speed = true;
    size_t trail_len = 40;
    bool follow_com = false;
    Preset preset = Preset::Swarm;

    sf::RenderWindow window(sf::VideoMode(width, height), "N-Body Gravity", sf::Style::Close);
    window.setFramerateLimit(60);

    sf::View view(sf::FloatRect(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)));
    window.setView(view);

    std::mt19937 rng(42);

    std::vector<Body> bodies;
    std::vector<std::deque<Vec2>> trails;

    auto reset_bodies = [&]() {
        apply_preset(preset, bodies, body_count, G, width, height, rng);
        trails.clear();
        trails.resize(static_cast<size_t>(body_count));
    };

    reset_bodies();
    load_config(config_path, body_count, G, softening, dt, show_trails, color_by_speed, trail_len);
    reset_bodies();

    bool paused = false;
    bool dragging = false;
    sf::Vector2i last_mouse;

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Space) paused = !paused;
                if (event.key.code == sf::Keyboard::R) {
                    preset = Preset::Swarm;
                    reset_bodies();
                }
                if (event.key.code == sf::Keyboard::T) show_trails = !show_trails;
                if (event.key.code == sf::Keyboard::C) color_by_speed = !color_by_speed;
                if (event.key.code == sf::Keyboard::Z && trail_len > 0) trail_len--;
                if (event.key.code == sf::Keyboard::X) trail_len++;
                if (event.key.code == sf::Keyboard::Equal) G = std::min(G + 20.0f, 2000.0f);
                if (event.key.code == sf::Keyboard::Hyphen) G = std::max(G - 20.0f, 1.0f);
                if (event.key.code == sf::Keyboard::LBracket) softening = std::max(softening - 0.5f, 0.1f);
                if (event.key.code == sf::Keyboard::RBracket) softening = std::min(softening + 0.5f, 50.0f);
                if (event.key.code == sf::Keyboard::Comma) dt = std::max(dt - 0.001f, 0.001f);
                if (event.key.code == sf::Keyboard::Period) dt = std::min(dt + 0.001f, 0.05f);
                if (event.key.code == sf::Keyboard::Num1) {
                    body_count = std::max(50, body_count - 25);
                    reset_bodies();
                }
                if (event.key.code == sf::Keyboard::Num2) {
                    body_count = std::min(400, body_count + 25);
                    reset_bodies();
                }
                if (event.key.code == sf::Keyboard::Num0) {
                    view = sf::View(sf::FloatRect(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)));
                    window.setView(view);
                }
                if (event.key.code == sf::Keyboard::F) {
                    follow_com = !follow_com;
                }
                if (event.key.code == sf::Keyboard::F1) {
                    preset = Preset::Galaxy;
                    reset_bodies();
                }
                if (event.key.code == sf::Keyboard::F2) {
                    preset = Preset::Ring;
                    reset_bodies();
                }
                if (event.key.code == sf::Keyboard::F3) {
                    preset = Preset::Swarm;
                    reset_bodies();
                }
                if (event.key.code == sf::Keyboard::S) {
                    save_config(config_path, body_count, G, softening, dt, show_trails, color_by_speed, trail_len);
                }
                if (event.key.code == sf::Keyboard::L) {
                    if (load_config(config_path, body_count, G, softening, dt, show_trails, color_by_speed, trail_len)) {
                        reset_bodies();
                    }
                }
            }
            if (event.type == sf::Event::MouseWheelScrolled) {
                if (event.mouseWheelScroll.delta > 0) view.zoom(0.9f);
                if (event.mouseWheelScroll.delta < 0) view.zoom(1.1f);
                window.setView(view);
            }
            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Right) {
                dragging = true;
                last_mouse = {event.mouseButton.x, event.mouseButton.y};
            }
            if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Right) {
                dragging = false;
            }
            if (event.type == sf::Event::MouseMoved && dragging) {
                sf::Vector2i cur = {event.mouseMove.x, event.mouseMove.y};
                sf::Vector2f world_last = window.mapPixelToCoords(last_mouse, view);
                sf::Vector2f world_cur = window.mapPixelToCoords(cur, view);
                sf::Vector2f delta = world_last - world_cur;
                view.move(delta);
                window.setView(view);
                last_mouse = cur;
            }
        }

        if (!paused) {
            std::vector<Vec2> acc = compute_accelerations(bodies, G, softening);

            // Velocity Verlet
            for (size_t i = 0; i < bodies.size(); ++i) {
                bodies[i].pos += bodies[i].vel * dt + acc[i] * (0.5f * dt * dt);
            }

            std::vector<Vec2> acc_new = compute_accelerations(bodies, G, softening);

            for (size_t i = 0; i < bodies.size(); ++i) {
                bodies[i].vel += (acc[i] + acc_new[i]) * (0.5f * dt);

                // Wrap around screen edges (world bounds)
                if (bodies[i].pos.x < 0) bodies[i].pos.x += width;
                if (bodies[i].pos.x >= width) bodies[i].pos.x -= width;
                if (bodies[i].pos.y < 0) bodies[i].pos.y += height;
                if (bodies[i].pos.y >= height) bodies[i].pos.y -= height;
            }
        }

        if (show_trails) {
            for (size_t i = 0; i < bodies.size(); ++i) {
                trails[i].push_back(bodies[i].pos);
                if (trails[i].size() > trail_len) trails[i].pop_front();
            }
        }

        if (follow_com) {
            Vec2 com = center_of_mass(bodies);
            view.setCenter(com.x, com.y);
            window.setView(view);
        }

        window.clear(sf::Color(8, 8, 12));

        // Draw trails
        if (show_trails) {
            for (size_t i = 0; i < bodies.size(); ++i) {
                if (trails[i].size() < 2) continue;
                sf::VertexArray strip(sf::LineStrip);
                strip.resize(trails[i].size());
                for (size_t t = 0; t < trails[i].size(); ++t) {
                    float alpha = static_cast<float>(t) / static_cast<float>(trails[i].size());
                    sf::Color col = color_by_speed ? speed_color(length(bodies[i].vel)) : sf::Color(180, 200, 255);
                    col.a = static_cast<sf::Uint8>(alpha * 180.0f);
                    strip[t].position = sf::Vector2f(trails[i][t].x, trails[i][t].y);
                    strip[t].color = col;
                }
                window.draw(strip);
            }
        }

        // Draw bodies
        for (size_t i = 0; i < bodies.size(); ++i) {
            float radius = std::sqrt(bodies[i].mass);
            sf::CircleShape circle(radius);
            sf::Color col = color_by_speed ? speed_color(length(bodies[i].vel)) : sf::Color(200, 220, 255);
            circle.setFillColor(col);
            circle.setPosition(bodies[i].pos.x - radius, bodies[i].pos.y - radius);
            window.draw(circle);
        }

        // UI overlay
        window.setView(window.getDefaultView());
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(3);
        oss << "G: " << G << "  SOFT: " << softening << "  DT: " << dt;
        draw_text(window, 12.0f, 12.0f, 2.0f, sf::Color(230, 230, 230), oss.str());

        std::ostringstream oss2;
        oss2 << "TRAIL: " << trail_len << "  BODIES: " << body_count
             << "  " << (paused ? "PAUSED" : "RUNNING")
             << "  COLOR: " << (color_by_speed ? "SPEED" : "FIXED")
             << "  PRESET: " << preset_name(preset)
             << "  FOLLOW: " << (follow_com ? "ON" : "OFF");
        draw_text(window, 12.0f, 32.0f, 2.0f, sf::Color(200, 200, 200), oss2.str());

        draw_text(window, 12.0f, 52.0f, 2.0f, sf::Color(160, 160, 160),
                  "SPACE PAUSE  R SWARM  T TRAILS  C COLOR  Z/X TRAIL LEN  +/- G  [/] SOFT  ,/. DT  1/2 BODIES  0 RESET VIEW");
        draw_text(window, 12.0f, 70.0f, 2.0f, sf::Color(160, 160, 160),
                  "F1 GALAXY  F2 RING  F3 SWARM  F FOLLOW COM  S SAVE CFG  L LOAD CFG  RMB DRAG  WHEEL ZOOM");

        window.setView(view);
        window.display();
    }

    return 0;
}
