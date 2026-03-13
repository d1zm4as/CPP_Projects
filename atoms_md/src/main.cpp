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

int main() {
    const unsigned width = 1000;
    const unsigned height = 700;

    const int count = 200;
    const float dt = 0.005f;
    const float sigma = 12.0f;     // LJ length scale
    const float epsilon = 2.0f;    // LJ depth
    const float cutoff = 3.0f * sigma;
    const float box_w = width;
    const float box_h = height;

    sf::RenderWindow window(sf::VideoMode(width, height), "Molecular Dynamics (Lennard-Jones)", sf::Style::Close);
    window.setFramerateLimit(60);

    std::mt19937 rng(123);
    std::uniform_real_distribution<float> posx(40.0f, width - 40.0f);
    std::uniform_real_distribution<float> posy(40.0f, height - 40.0f);
    std::uniform_real_distribution<float> veld(-30.0f, 30.0f);

    std::vector<Particle> particles;
    particles.reserve(count);

    for (int i = 0; i < count; ++i) {
        Particle p;
        p.pos = {posx(rng), posy(rng)};
        p.vel = {veld(rng), veld(rng)};
        particles.push_back(p);
    }

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) window.close();
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

        for (const auto& p : particles) {
            sf::CircleShape c(2.5f);
            c.setFillColor(sf::Color(180, 220, 255));
            c.setPosition(p.pos.x - 2.5f, p.pos.y - 2.5f);
            window.draw(c);
        }

        window.display();
    }

    return 0;
}
