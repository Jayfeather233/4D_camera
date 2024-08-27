#include "object_base.hpp"

#include <vector>
#include <GL/glew.h>

#define pushback4(a, b, c, d, e) \
{a.push_back(b);\
a.push_back(c);\
a.push_back(d);\
a.push_back(e);}

#define pushback3(a, b, c, d) \
{a.push_back(b);\
a.push_back(c);\
a.push_back(d);}

#define pushback2(a, b, c) \
{a.push_back(b);\
a.push_back(c);}

obj_base::obj_base(std::vector<float> pts, std::vector<GLuint> eg) : points(pts), edges(eg) {}

obj_base *create_obj_base(object_type tp)
{
    if (tp == object_type::CUBE_4D)
    {
        obj_base *obj = new obj_base({
            0, 0, 0, 0, 0.8f, 0.8f, 0.0f,
            1, 0, 0, 0, 0.8f, 0.8f, 0.0f,
            0, 1, 0, 0, 0.8f, 0.8f, 0.0f,
            1, 1, 0, 0, 0.8f, 0.8f, 0.0f,
            0, 0, 1, 0, 0.0f, 0.8f, 0.8f,
            1, 0, 1, 0, 0.0f, 0.8f, 0.8f,
            0, 1, 1, 0, 0.0f, 0.8f, 0.8f,
            1, 1, 1, 0, 0.0f, 0.8f, 0.8f,
            0, 0, 0, 1, 0.8f, 0.0f, 0.8f,
            1, 0, 0, 1, 0.8f, 0.0f, 0.8f,
            0, 1, 0, 1, 0.8f, 0.0f, 0.8f,
            1, 1, 0, 1, 0.8f, 0.0f, 0.8f,
            0, 0, 1, 1, 0.8f, 0.0f, 0.0f,
            1, 0, 1, 1, 0.8f, 0.0f, 0.0f,
            0, 1, 1, 1, 0.8f, 0.0f, 0.0f,
            1, 1, 1, 1, 0.8f, 0.0f, 0.0f,
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
            0.0, 0.0, 0.0, 1.0f, 1.0f, 1.0f,
            1.0, 0.0, 0.0, 1.0f, 1.0f, 1.0f,
            0.0, 1.0, 0.0, 1.0f, 1.0f, 1.0f,
            1.0, 1.0, 0.0, 1.0f, 1.0f, 1.0f,
            0.0, 0.0, 1.0, 1.0f, 1.0f, 1.0f,
            1.0, 0.0, 1.0, 1.0f, 1.0f, 1.0f,
            0.0, 1.0, 1.0, 1.0f, 1.0f, 1.0f,
            1.0, 1.0, 1.0, 1.0f, 1.0f, 1.0f,
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
    } else if (tp == object_type::DUO_CYLINDER){
        int N = 12;
        size_t pts_id = 0;
        obj_base *obj = new obj_base({}, {});
        for(int i = 0; i < N; ++i){
            double angle = i*1.0/N * glm::two_pi<double>();
            double x = glm::cos(angle);
            double y = glm::sin(angle);
            for(int j = 0; j < N; j++){
                double angle2 = j*1.0/N * glm::two_pi<double>();
                double z = glm::cos(angle2);
                double w = glm::sin(angle2);
                pushback4(obj->points, x, y, z, w);
                pushback3(obj->points, 1.0, 1.0, 1.0);

                if(j != 0) pushback2(obj->edges, pts_id, pts_id - 1);
                if(j == N-1) pushback2(obj->edges, pts_id, pts_id - N + 1);

                pts_id ++;
            }
            if(i != 0){
                for(int j = 0; j < N; ++j){
                    pushback2(obj->edges, pts_id - j - 1, pts_id - j - N - 1);
                }
            }
            if(i == N-1){
                for(int j = 0; j < N; ++j){
                    pushback2(obj->edges, pts_id - N + j, j);
                }
            }
        }
        return obj;
    } else if (tp == object_type::SPHERE_4D){
        int N = 12;
        size_t pts_id = 0;
        obj_base *obj = new obj_base({}, {});
        size_t las_cir1 = pts_id;
        for(int i=0;i<N;++i){
            double angle1 = i*1.0/N * glm::two_pi<double>();
            double c1 = glm::cos(angle1);
            double s1 = glm::sin(angle1);

            size_t las_cir2 = pts_id;
            for(int j=0;j<N;++j){
                double angle2 = j*1.0/N * glm::two_pi<double>();
                double c2 = glm::cos(angle2);
                double s2 = glm::sin(angle2);
                for(int k=0;k<N;++k){
                    double angle3 = k*1.0/N * glm::two_pi<double>();
                    double c3 = glm::cos(angle3);
                    double s3 = glm::sin(angle3);

                    double x = c1;
                    double y = s1 * c2;
                    double z = s1 * s2 * c3;
                    double w = s1 * s2 * s3;

                    pushback4(obj->points, x, y, z, w);
                    pushback3(obj->points, i*1.0/N, j*1.0/N, k*1.0/N);
                    if(k != 0) pushback2(obj->edges, pts_id, pts_id - 1);
                    if(k == N-1) pushback2(obj->edges, pts_id, pts_id - N + 1);
                    pts_id ++;
                }
                if(j != 0){
                    for(int k = 1; k <= N; ++k){
                        pushback2(obj->edges, pts_id - k, las_cir2 - k);
                    }
                }
                las_cir2 = pts_id;
            }
            size_t N2 = N * N;
            if(i != 0){
                for(int k = 1; k <= N2; ++k){
                    pushback2(obj->edges, pts_id - k, las_cir1 - k);
                }
            }
            las_cir1 = pts_id;
        }
        return obj;
    } else if (tp == object_type::PLANE_4D){
        obj_base *obj = new obj_base({}, {});
        size_t pts_id = 0;
        int N = 20;
        for(int i=0;i<=N;++i){
            for(int j=0;j<=N;++j){
                pushback4(obj->points, (i-N/2)/2.0, (j-N/2)/2.0, 0.0, 0.0);
                pushback3(obj->points, 1.0, 1.0, 1.0);
                if(j!=0){
                    pushback2(obj->edges, pts_id, pts_id - 1);
                }
                if(i!=0){
                    pushback2(obj->edges, pts_id, pts_id - N - 1);
                }
                pts_id ++;
            }
        }
        return obj;
    } else if (tp == object_type::SUPERPLANE_4D){
        obj_base *obj = new obj_base({}, {});
        size_t pts_id = 0;
        int N = 20;
        for(int i=0;i<=N;++i){
            for(int j=0;j<=N;++j){
                for(int k=0;k<=N;++k){
                    pushback4(obj->points, (i-N/2)/2.0, (j-N/2)/2.0, (k-N/2)/2.0, 0.0);
                    pushback3(obj->points, 1.0, 1.0, 1.0);
                    if(k!=0){
                        pushback2(obj->edges, pts_id, pts_id - 1);
                    }
                    if(j!=0){
                        pushback2(obj->edges, pts_id, pts_id - N - 1);
                    }
                    if(i!=0){
                        pushback2(obj->edges, pts_id, pts_id - (N+1)*(N+1));
                    }
                pts_id ++;
                }
            }
        }
        return obj;
    }
    return nullptr;
}