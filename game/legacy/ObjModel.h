#pragma once
#include <string>
#include <vector>

namespace ert {

struct Vertex {
    float x, y, z;
    float nx, ny, nz; // Normals
};

// Haqiqiy 3D (.OBJ) o'qigich! Ertug'rul va Daraxtni yuklaydi.
class ObjModel {
public:
    bool load(const std::string& filepath);
    void draw() const;

private:
    std::vector<Vertex> vertices_;
};

}
