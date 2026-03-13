#include <SFML/Graphics.hpp>
#include <cmath>
#include <random>
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

static float length(const Vec2& v) {
    return std::sqrt(v.x * v.x + v.y * v.y);
}

struct Particle {
    Vec2 pos;
    Vec2 vel;
};

static float rand_range(std::mt19937& rng, float a, float b) {
    std::uniform_real_distribution<float> dist(a, b);
    return dist(rng);
}

static sf::Color speed_color(float speed) {
    float t = std::min(speed / 120.0f, 1.0f);
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

int main() {
    const unsigned width = 1000;
    const unsigned height = 700;

    int count = 200;
    const float dt = 0.005f;
    const float sigma = 12.0f;     // LJ length scale
    const float epsilon = 2.0f;    // LJ depth
    const float cutoff = 3.0f * sigma;
    const float box_w = width;
    const float box_h = height;
    const float bond_dist = 2.2f * sigma;

    sf::RenderWindow window(sf::VideoMode(width, height), "Molecular Dynamics (Lennard-Jones)", sf::Style::Close);
    window.setFramerateLimit(60);

    std::mt19937 rng(123);

    std::vector<Particle> particles;
    auto reset_particles = [&](int new_count) {
        particles.clear();
        particles.reserve(new_count);
        for (int i = 0; i < new_count; ++i) {
            Particle p;
            p.pos = {rand_range(rng, 40.0f, width - 40.0f), rand_range(rng, 40.0f, height - 40.0f)};
            p.vel = {rand_range(rng, -30.0f, 30.0f), rand_range(rng, -30.0f, 30.0f)};
            particles.push_back(p);
        }
    };

    reset_particles(count);

    bool color_by_speed = true;
    bool draw_bonds = true;

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) window.close();
            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::C) color_by_speed = !color_by_speed;
                if (event.key.code == sf::Keyboard::B) draw_bonds = !draw_bonds;
                if (event.key.code == sf::Keyboard::H) {
                    for (auto& p : particles) p.vel *= 1.1f; // heat up
                }
                if (event.key.code == sf::Keyboard::J) {
                    for (auto& p : particles) p.vel *= 0.9f; // cool down
                }
                if (event.key.code == sf::Keyboard::Num1) {
                    count = std::max(50, count - 50);
                    reset_particles(count);
                }
                if (event.key.code == sf::Keyboard::Num2) {
                    count = std::min(600, count + 50);
                    reset_particles(count);
                }
            }
        }

        // Compute forces
        std::vector<Vec2> forces(particles.size(), {0.0f, 0.0f});
        for (size_t i = 0; i < particles.size(); ++i) {
            for (size_t j = i + 1; j < particles.size(); ++j) {
                Vec2 r = particles[j].pos - particles[i].pos;
                float dist = length(r);
                if (dist < 1e-6f || dist > cutoff) continue;

                float inv = sigma / dist;
                float inv2 = inv * inv;
                float inv6 = inv2 * inv2 * inv2;
                float inv12 = inv6 * inv6;
                float fmag = 24.0f * epsilon * (2.0f * inv12 - inv6) / dist;

                Vec2 f = r * (fmag / dist);
                forces[i] -= f;
                forces[j] += f;
            }
        }

        // Integrate (simple velocity verlet)
        for (size_t i = 0; i < particles.size(); ++i) {
            particles[i].vel += forces[i] * dt;
            particles[i].pos += particles[i].vel * dt;

            // Periodic boundary conditions
            if (particles[i].pos.x < 0) particles[i].pos.x += box_w;
            if (particles[i].pos.x >= box_w) particles[i].pos.x -= box_w;
            if (particles[i].pos.y < 0) particles[i].pos.y += box_h;
            if (particles[i].pos.y >= box_h) particles[i].pos.y -= box_h;
        }

        window.clear(sf::Color(8, 8, 12));

        if (draw_bonds) {
            sf::VertexArray bonds(sf::Lines);
            bonds.clear();
            for (size_t i = 0; i < particles.size(); ++i) {
                for (size_t j = i + 1; j < particles.size(); ++j) {
                    Vec2 r = particles[j].pos - particles[i].pos;
                    float dist = length(r);
                    if (dist < bond_dist) {
                        float alpha = 1.0f - (dist / bond_dist);
                        sf::Color col(120, 140, 180, static_cast<sf::Uint8>(alpha * 120.0f));
                        bonds.append(sf::Vertex(sf::Vector2f(particles[i].pos.x, particles[i].pos.y), col));
                        bonds.append(sf::Vertex(sf::Vector2f(particles[j].pos.x, particles[j].pos.y), col));
                    }
                }
            }
            window.draw(bonds);
        }

        for (const auto& p : particles) {
            sf::CircleShape c(2.5f);
            if (color_by_speed) c.setFillColor(speed_color(length(p.vel)));
            else c.setFillColor(sf::Color(180, 220, 255));
            c.setPosition(p.pos.x - 2.5f, p.pos.y - 2.5f);
            window.draw(c);
        }

        draw_text(window, 12.0f, 10.0f, 2.0f, sf::Color(220, 220, 230),
                  "H/J TEMP  1/2 COUNT  C COLOR  B BONDS");

        window.display();
    }

    return 0;
}
