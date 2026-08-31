#include "ertugrul/app/ObjModel.h"
#include <windows.h>
#include <gl/gl.h>
#include <iostream>
#include <fstream>
#include <sstream>

namespace ert {

bool ObjModel::load(const std::string& filepath) {
    vertices_.clear();
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "3D Model topilmadi: " << filepath << "\n";
        return false;
    }

    std::string line;
    std::vector<Vertex> temp_vertices;
    std::vector<Vertex> temp_normals;

    while (std::getline(file, line)) {
        if (line.substr(0, 2) == "v ") {
            std::istringstream s(line.substr(2));
            Vertex v;
            v.nx = 0; v.ny = 1; v.nz = 0; // Default normal
            s >> v.x >> v.y >> v.z;
            temp_vertices.push_back(v);
        }
        else if (line.substr(0, 3) == "vn ") {
            std::istringstream s(line.substr(3));
            Vertex n;
            s >> n.x >> n.y >> n.z;
            temp_normals.push_back(n);
        }
        else if (line.substr(0, 2) == "f ") {
            std::istringstream s(line.substr(2));
            std::string a, b, c;
            s >> a >> b >> c;
            
            auto parseFace = [&](const std::string& str) {
                int v_idx = 0, vn_idx = -1;
                size_t first_slash = str.find('/');
                if (first_slash != std::string::npos) {
                    v_idx = std::stoi(str.substr(0, first_slash)) - 1;
                    size_t second_slash = str.find('/', first_slash + 1);
                    if (second_slash != std::string::npos && second_slash + 1 < str.length()) {
                        vn_idx = std::stoi(str.substr(second_slash + 1)) - 1;
                    }
                } else {
                    v_idx = std::stoi(str) - 1;
                }
                
                Vertex v = temp_vertices[v_idx];
                if (vn_idx >= 0 && vn_idx < temp_normals.size()) {
                    v.nx = temp_normals[vn_idx].x;
                    v.ny = temp_normals[vn_idx].y;
                    v.nz = temp_normals[vn_idx].z;
                }
                return v;
            };

            if (!temp_vertices.empty()) {
                vertices_.push_back(parseFace(a));
                vertices_.push_back(parseFace(b));
                vertices_.push_back(parseFace(c));
            }
        }
    }
    std::cout << filepath << " dan " << vertices_.size() / 3 << " ta poligon yuklandi!\n";
    return true;
}

void ObjModel::draw() const {
    if (vertices_.empty()) return;
    
    glBegin(GL_TRIANGLES);
    for (const auto& v : vertices_) {
        glNormal3f(v.nx, v.ny, v.nz);
        glVertex3f(v.x, v.y, v.z);
    }
    glEnd();
}

} // namespace ert
