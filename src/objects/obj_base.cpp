#include "object_base.hpp"

#include <GL/glew.h>
#include <vector>

#define pushback4(a, b, c, d, e)                                                                                       \
    do {                                                                                                               \
        a.push_back(b);                                                                                                \
        a.push_back(c);                                                                                                \
        a.push_back(d);                                                                                                \
        a.push_back(e);                                                                                                \
    } while (0)

#define pushback3(a, b, c, d)                                                                                          \
    do {                                                                                                               \
        a.push_back(b);                                                                                                \
        a.push_back(c);                                                                                                \
        a.push_back(d);                                                                                                \
    } while (0)

#define pushback2(a, b, c)                                                                                             \
    do {                                                                                                               \
        a.push_back(b);                                                                                                \
        a.push_back(c);                                                                                                \
    } while (0)

auto identity_scomplex24 = [](const Eigen::scomplex &u) { return Eigen::Vector4f(u.real(), u.imag(), 0.0, 0.0); };
auto ideneity_color23 = [](const Eigen::scomplex &u) { return Eigen::Vector3f(u.real(), u.imag(), 1.0); };

auto identity_scomplex34 = [](const Eigen::Vector3f &u) { return Eigen::Vector4f(u.x(), u.y(), u.z(), 0.0); };
auto ideneity_color33 = [](const Eigen::Vector3f &u) {
    return Eigen::Vector3f(u.x() * 0.9 + 0.1, u.y() * 0.9 + 0.1, u.z() * 0.9 + 0.1);
};

/**
 * N: bigger the smoother
 * f(x, y)->(x, y, z, w),
 * -D <= x, y <= D
 * TODO: N, D -> Eigen:Vec3f
 */
obj_base *create_hyper_plane(std::function<Eigen::Vector4f(const Eigen::scomplex &)> f,
                             int N, Eigen::Vector2f D,
                             std::function<Eigen::Vector3f(const Eigen::scomplex &)> color = ideneity_color23,
                             uint32_t circle_around = 0xffffffff)
{
    obj_base *obj = new obj_base({}, {});
    size_t pts_id = 0;
    size_t N_1 = N - 1;
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            Eigen::scomplex ori = Eigen::scomplex((i - N_1 / 2.0) / N_1 * 2 * D.x(), (j - N_1 / 2.0) / N_1 * 2 * D.y());
            Eigen::Vector4f a = f(ori);
            Eigen::Vector3f col = color(Eigen::scomplex(i * 1.0 / N_1, j * 1.0 / N_1));
            pushback4(obj->points, a.x(), a.y(), a.z(), a.w());
            pushback3(obj->points, col.x(), col.y(), col.z());
            if (j != 0) {
                pushback2(obj->edges, pts_id, pts_id - 1);
            }
            if (i != 0) {
                pushback2(obj->edges, pts_id, pts_id - N);
            }
            if ((circle_around & 1) && j == N - 1) {
                pushback2(obj->edges, pts_id, pts_id - N + 1);
            }
            pts_id++;
        }
        if ((circle_around & 2) && i == N - 1) {
            for (int j = 0; j < N; ++j) {
                pushback2(obj->edges, pts_id - N + 1 + j, j);
            }
        }
    }
    return obj;
}
obj_base *create_hyper_plane(std::function<Eigen::Vector4f(const Eigen::scomplex &)> f = identity_scomplex24,
                             int N = 20, float D = 1.0,
                             std::function<Eigen::Vector3f(const Eigen::scomplex &)> color = ideneity_color23,
                             uint32_t circle_around = 0xffffffff) {
    return create_hyper_plane(f, N, Eigen::Vector2f(D, D), color, circle_around);
}

obj_base *create_cube(std::function<Eigen::Vector4f(const Eigen::Vector3f &)> f, int N, Eigen::Vector3f D,
                      std::function<Eigen::Vector3f(const Eigen::Vector3f &)> color = ideneity_color33,
                      uint32_t circle_around = 0xffffffff)
{
    obj_base *obj = new obj_base({}, {});
    size_t pts_id = 0;
    size_t N_1 = N - 1;
    size_t N2 = N * N;
    size_t N3 = N2 * N;
    size_t N_1N = (N - 1) * N;
    size_t N_1N2 = (N - 1) * N * N;
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            for (int k = 0; k < N; ++k) {
                Eigen::Vector3f ori =
                    Eigen::Vector3f((i - N_1 / 2.0) / N_1 * 2 * D.x(), (j - N_1 / 2.0) / N_1 * 2 * D.y(),
                                    (k - N_1 / 2.0) / N_1 * 2 * D.z());
                Eigen::Vector4f a = f(ori);
                Eigen::Vector3f col = color(Eigen::Vector3f(i * 1.0 / N_1, j * 1.0 / N_1, k * 1.0 / N_1));

                pushback4(obj->points, a.x(), a.y(), a.z(), a.w());
                pushback3(obj->points, col.x(), col.y(), col.z());
                if (k != 0) {
                    pushback2(obj->edges, pts_id, pts_id - 1);
                }
                if (j != 0) {
                    pushback2(obj->edges, pts_id, pts_id - N);
                }
                if (i != 0) {
                    pushback2(obj->edges, pts_id, pts_id - N2);
                }

                if ((circle_around & 1) != 0 && k == N - 1) {
                    pushback2(obj->edges, pts_id, pts_id - N + 1);
                }
                if ((circle_around & 2) != 0 && j == N - 1) {
                    pushback2(obj->edges, pts_id, pts_id - N_1N);
                }
                if ((circle_around & 4) != 0 && i == N - 1) {
                    pushback2(obj->edges, pts_id, pts_id - N_1N2);
                }
                pts_id++;
            }
        }
    }
    return obj;
}

obj_base *create_cube(std::function<Eigen::Vector4f(const Eigen::Vector3f &)> f = identity_scomplex34, int N = 20,
                      float D = 1.0, std::function<Eigen::Vector3f(const Eigen::Vector3f &)> color = ideneity_color33,
                      uint32_t circle_around = 0xffffffff)
{
    return create_cube(f, N, Eigen::Vector3f(D, D, D), color, circle_around);
}

obj_base::obj_base(std::vector<float> pts, std::vector<GLuint> eg) : points(pts), edges(eg) {}

obj_base *create_obj_base(object_type tp)
{
    if (tp == object_type::CUBE_4D)
    {
        obj_base *obj = new obj_base({
            -0.5, -0.5, -0.5, -0.5, 0.8f, 0.8f, 0.0f,
             0.5, -0.5, -0.5, -0.5, 0.8f, 0.8f, 0.0f,
            -0.5,  0.5, -0.5, -0.5, 0.8f, 0.8f, 0.0f,
             0.5,  0.5, -0.5, -0.5, 0.8f, 0.8f, 0.0f,
            -0.5, -0.5,  0.5, -0.5, 0.0f, 0.8f, 0.8f,
             0.5, -0.5,  0.5, -0.5, 0.0f, 0.8f, 0.8f,
            -0.5,  0.5,  0.5, -0.5, 0.0f, 0.8f, 0.8f,
             0.5,  0.5,  0.5, -0.5, 0.0f, 0.8f, 0.8f,
            -0.5, -0.5, -0.5,  0.5, 0.8f, 0.0f, 0.8f,
             0.5, -0.5, -0.5,  0.5, 0.8f, 0.0f, 0.8f,
            -0.5,  0.5, -0.5,  0.5, 0.8f, 0.0f, 0.8f,
             0.5,  0.5, -0.5,  0.5, 0.8f, 0.0f, 0.8f,
            -0.5, -0.5,  0.5,  0.5, 0.8f, 0.0f, 0.0f,
             0.5, -0.5,  0.5,  0.5, 0.8f, 0.0f, 0.0f,
            -0.5,  0.5,  0.5,  0.5, 0.8f, 0.0f, 0.0f,
             0.5,  0.5,  0.5,  0.5, 0.8f, 0.0f, 0.0f,
        },{
            0, 1,
            0, 2,
            1, 3,
            2, 3,
            
            4, 5,
            4, 6,
            5, 7,
            6, 7,
            
            8, 9,
            8, 10,
            9, 11,
            10, 11,
            
            12, 13,
            12, 14,
            13, 15,
            14, 15,

            0, 4,
            1, 5,
            2, 6,
            3, 7,

            0, 8,
            1, 9,
            2, 10,
            3, 11,

            8, 12,
            9, 13,
            10, 14,
            11, 15,

            4, 12,
            5, 13,
            6, 14,
            7, 15
        });
        return obj;
    }
    else if (tp == object_type::CUBE_3D)
    {
        obj_base *obj = new obj_base({
            -0.5, -0.5, -0.5, 1.0f, 1.0f, 1.0f,
             0.5, -0.5, -0.5, 1.0f, 1.0f, 1.0f,
            -0.5,  0.5, -0.5, 1.0f, 1.0f, 1.0f,
             0.5,  0.5, -0.5, 1.0f, 1.0f, 1.0f,
            -0.5, -0.5,  0.5, 1.0f, 1.0f, 1.0f,
             0.5, -0.5,  0.5, 1.0f, 1.0f, 1.0f,
            -0.5,  0.5,  0.5, 1.0f, 1.0f, 1.0f,
             0.5,  0.5,  0.5, 1.0f, 1.0f, 1.0f,
        },{
            0, 1,
            0, 2,
            1, 3,
            2, 3,
            
            4, 5,
            4, 6,
            5, 7,
            6, 7,
            
            0, 4,
            1, 5,
            2, 6,
            3, 7,
        });
        return obj;
    } else if (tp == object_type::COORD_4D){
        obj_base *obj = new obj_base({
            20, 0, 0, 0, 1.0f, 1.0f, 1.0f,
            -20, 0, 0, 0, 1.0f, 1.0f, 1.0f,
            0, 20, 0, 0, 1.0f, 1.0f, 1.0f,
            0, -20, 0, 0, 1.0f, 1.0f, 1.0f,
            0, 0, 20, 0, 1.0f, 1.0f, 1.0f,
            0, 0, -20, 0, 1.0f, 1.0f, 1.0f,
            0, 0, 0, 20, 1.0f, 1.0f, 1.0f,
            0, 0, 0, -20, 1.0f, 1.0f, 1.0f,
        },{
            0, 1,
            2, 3,
            4, 5,
            6, 7
        });
        return obj;
    } else if (tp == object_type::CELL5){
        obj_base *obj = new obj_base({
            0, 0, 2.1213, -0.5477, 1.0f, 1.0f, 1.0f,
            -1, 1.7320, -0.7071, -0.5477, 1.0f, 1.0f, 1.0f,
            -1, -1.7320, -0.7071, -0.5477, 1.0f, 1.0f, 1.0f,
            2, 0, -0.7071, -0.5477, 1.0f, 1.0f, 1.0f,
            0, 0, 0, 2.1909, 1.0f, 1.0f, 1.0f,
        },{
            0, 1,
            0, 2,
            0, 3,
            0, 4,
            1, 2,
            1, 3,
            1, 4,
            2, 3,
            2, 4,
            3, 4
        });
        return obj;
    }
    else if (tp == object_type::DUO_CYLINDER) {
        return create_hyper_plane(
            [](const Eigen::scomplex &a) {
                return Eigen::Vector4f(glm::cos(a.real()), glm::sin(a.real()), glm::cos(a.imag()), glm::sin(a.imag()));
            },
            100, glm::pi<float>(),
            [](const Eigen::scomplex &a) {
                return a.real() < 0 ? (a.imag() < 0 ? Eigen::Vector3f(1.0, 1.0, 0.0) : Eigen::Vector3f(1.0, 0.0, 1.0))
                                    : (a.imag() < 0 ? Eigen::Vector3f(0.0, 1.0, 1.0) : Eigen::Vector3f(1.0, 1.0, 1.0));
            });
    }
    else if (tp == object_type::SPHERE_4D) {
        return create_cube(
            [](const Eigen::Vector3f &a) {
                float c1 = glm::cos(a.x() + glm::pi<float>() / 2);
                float s1 = glm::sin(a.x() + glm::pi<float>() / 2);
                float c2 = glm::cos(a.y() + glm::pi<float>() / 2);
                float s2 = glm::sin(a.y() + glm::pi<float>() / 2);
                float c3 = glm::cos(a.z());
                float s3 = glm::sin(a.z());
                return Eigen::Vector4f(c1, s1 * c2, s1 * s2 * c3, s1 * s2 * s3);
            },
            9, Eigen::Vector3f(glm::pi<float>() / 2, glm::pi<float>() / 2, glm::pi<float>()),
            [](const Eigen::Vector3f &a) { return Eigen::Vector3f(1.0f, 1.0f, 1.0f); }, 1);
    }
    else if (tp == object_type::PLANE_4D) {
        return create_hyper_plane(identity_scomplex24, 20, 5);
    }
    else if (tp == object_type::SUPERPLANE_4D) {
        return create_cube(identity_scomplex34, 20, 1.0, ideneity_color33, 0);
    }
    else if (tp == object_type::SPRING_4D) {
        float r = 0.3, R = 1, C = 1;
        return create_cube(
            [&](const Eigen::Vector3f &a) {
                return Eigen::Vector4f(R*glm::cos(a.z()) + r*glm::cos(a.x()+glm::pi<float>()/2), R*glm::sin(a.z()) + r*glm::sin(a.x()+glm::pi<float>()/2)*glm::cos(a.y()), R*glm::sin(a.z()) + r*glm::sin(a.x()+glm::pi<float>()/2)*glm::sin(a.y()), C * a.z());
            }, 20, Eigen::Vector3f(glm::pi<float>()/2, glm::pi<float>(), 10), ideneity_color33, 6
        );
    }
    else if (tp == object_type::CYLINDER_PRISM) {
        obj_base *obj = new obj_base({}, {});
        size_t pts_id = 0;
        int N = 20;
        size_t las1, las2;
        for (int i = 0; i <= 2; ++i) {
            las1 = pts_id;
            for (int j = 0; j <= 2; ++j) {
                las2 = pts_id;
                for (int k = 0; k <= N; ++k) {
                    double angle3 = k * 1.0 / N * glm::two_pi<double>();
                    double c3 = glm::cos(angle3);
                    double s3 = glm::sin(angle3);
                    // pushback4(obj->points, c3, s3, i-0.5, j-0.5);
                    pushback4(obj->points, i - 1, j - 1, c3, s3);
                    pushback3(obj->points, 1.0, 1.0, 1.0);
                    if (k != 0)
                        pushback2(obj->edges, pts_id, pts_id - 1);
                    if (k == N - 1)
                        pushback2(obj->edges, pts_id, pts_id - N + 1);
                    pts_id++;
                }
                if (j != 0) {
                    for (size_t off = 1; off <= pts_id - las2; ++off) {
                        pushback2(obj->edges, pts_id - off, las2 - off);
                    }
                }
            }
            if (i != 0) {
                for (size_t off = 1; off <= pts_id - las1; ++off) {
                    pushback2(obj->edges, pts_id - off, las1 - off);
                }
            }
        }
        return obj;
    }
    else if (tp == object_type::HYPER_PARABOLA) {
        return create_hyper_plane(
            [](const Eigen::scomplex &a) {
                auto u1 = a * a;
                return Eigen::Vector4f(a.real(), a.imag(), u1.real(), u1.imag());
            },
            100, 2.0);
    }
    else if (tp == object_type::HYPER_EX) {
        return create_hyper_plane(
            [](const Eigen::scomplex &a) {
                auto u1 = std::exp(a);
                return Eigen::Vector4f(a.real(), a.imag(), u1.real(), u1.imag());
            },
            100, glm::two_pi<float>() * 2);
    }

    return nullptr;
}