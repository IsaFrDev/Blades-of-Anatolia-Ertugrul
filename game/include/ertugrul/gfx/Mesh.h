#pragma once
// OBJ + MTL yuklovchi. Uchburchak/to'rtburchak/n-burchak, UV, normal, material guruhlari.
#include <string>
#include <vector>
#include "ertugrul/core/Math.h"

namespace ert {

class Texture;

struct MeshVertex {
    float px = 0, py = 0, pz = 0;
    float nx = 0, ny = 1, nz = 0;
    float u  = 0, v  = 0;
};

struct SubMesh {
    size_t      first = 0;     // vertices_ ichidagi boshlang'ich indeks
    size_t      count = 0;     // vertekslar soni (3 ga karrali)
    std::string material;
    Texture*    tex  = nullptr;   // map_Kd (yo'q bo'lsa nullptr)
    float       kd[3] = {1,1,1};  // diffuz rang
    float       alpha = 1.0f;
};

class Mesh {
public:
    Mesh() = default;
    ~Mesh();
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    // .obj (yonidagi .mtl avtomatik) yuklaydi. Xato matni err ga yoziladi.
    bool load(const std::string& objPath, std::string* err = nullptr);
    void destroy();

    bool   valid() const { return !vertices_.empty(); }
    size_t triangleCount() const { return vertices_.size() / 3; }
    size_t vertexCount()   const { return vertices_.size(); }

    const std::vector<MeshVertex>& vertices() const { return vertices_; }
    const std::vector<SubMesh>&    subs()     const { return subs_; }

    Vec3  bbMin()  const { return bbMin_; }
    Vec3  bbMax()  const { return bbMax_; }
    Vec3  center() const { return (bbMin_ + bbMax_) * 0.5f; }
    float height() const { return bbMax_.y - bbMin_.y; }

    // O'zining vertekslari bilan chizadi (klient massivlar; tez yo'l).
    void draw() const;
    // Tashqi (deformatsiyalangan) vertekslar bilan chizadi. Kattaligi vertexCount() ga teng bo'lishi shart.
    void drawDeformed(const std::vector<MeshVertex>& deformed) const;

    // Statik geometriya uchun display list (bir marta quriladi, keyin drawList()).
    void buildDisplayList();
    void drawList() const;
    bool hasList() const { return list_ != 0; }

    // --- Kesh ---
    static Mesh* get(const std::string& objPath);   // yuklab bo'lmasa nullptr
    // Mavjud meshning DEFORMATSIYALANGAN nusxasi (keshlanadi).
    // variant: "child" — bola nisbatlari: oyoq qisqa, tana biroz qisqa, bosh katta.
    // Kattalar modelini shunchaki kichraytirsak, "mitti kattalar" bo'lib qoladi;
    // nisbatlar o'zgarsa haqiqiy bola siluetiga yaqinlashadi.
    static Mesh* getVariant(const std::string& objPath, const std::string& variant);
    static void  clearCache();

    // Protsedural primitivlar (fayl kerak emas)
    static Mesh* unitCube();
    static Mesh* unitCylinder(int segments = 12);
    static Mesh* unitCone(int segments = 12);
    static Mesh* unitSphere(int rings = 12, int segs = 16);
    static Mesh* quadXZ();

private:
    std::vector<MeshVertex> vertices_;
    std::vector<SubMesh>    subs_;
    Vec3     bbMin_{0,0,0}, bbMax_{0,0,0};
    unsigned list_ = 0;
    std::string path_;
};

} // namespace ert
