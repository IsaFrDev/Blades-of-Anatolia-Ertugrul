// Mesh.cpp — OBJ/MTL yuklovchi va tez chizuvchi.
// Immediate rejim (glBegin/glEnd) ISHLATILMAYDI: ~100k uchburchakli modellar uchun
// faqat klient verteks massivlari + glDrawArrays ishlatiladi.
#include "ertugrul/gfx/Mesh.h"
#include "ertugrul/gfx/Texture.h"

#include <windows.h>   // <GL/gl.h> dan OLDIN kelishi SHART
#include <GL/gl.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <map>
#include <string>
#include <vector>
#include <algorithm>
#include <new>

namespace ert {

namespace {

// ---------------------------------------------------------------------------
//  Umumiy yordamchilar
// ---------------------------------------------------------------------------

// Faylni to'liq xotiraga o'qiydi (oxiriga '\0' qo'shadi).
bool readWholeFile(const std::string& path, std::vector<char>& out) {
    out.clear();
    if (path.empty()) return false;
    FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) return false;
    if (std::fseek(f, 0, SEEK_END) != 0) { std::fclose(f); return false; }
    long sz = std::ftell(f);
    if (sz < 0) { std::fclose(f); return false; }
    std::rewind(f);
    out.resize(static_cast<size_t>(sz) + 1u);
    size_t rd = 0;
    if (sz > 0) rd = std::fread(out.data(), 1, static_cast<size_t>(sz), f);
    std::fclose(f);
    out.resize(rd + 1u);
    out[rd] = '\0';
    return true;
}

bool fileExists(const std::string& p) {
    if (p.empty()) return false;
    DWORD a = ::GetFileAttributesA(p.c_str());
    return (a != INVALID_FILE_ATTRIBUTES) && !(a & FILE_ATTRIBUTE_DIRECTORY);
}

// Yo'lning papka qismi (oxirida ajratgich bilan). Papka bo'lmasa bo'sh satr.
std::string dirOf(const std::string& p) {
    size_t s1 = p.find_last_of('/');
    size_t s2 = p.find_last_of('\\');
    size_t s = std::string::npos;
    if (s1 == std::string::npos)      s = s2;
    else if (s2 == std::string::npos) s = s1;
    else                              s = (s1 > s2 ? s1 : s2);
    if (s == std::string::npos) return std::string();
    return p.substr(0, s + 1);
}

// Yo'lning fayl nomi qismi.
std::string baseOf(const std::string& p) {
    std::string d = dirOf(p);
    return d.empty() ? p : p.substr(d.size());
}

// Windows va Unix ajratgichlarini '/' ga keltiradi.
std::string toSlash(const std::string& p) {
    std::string r = p;
    for (char& c : r) if (c == '\\') c = '/';
    return r;
}

std::string trimBoth(const std::string& s) {
    size_t a = 0, b = s.size();
    while (a < b && (s[a] == ' ' || s[a] == '\t' || s[a] == '\r' || s[a] == '\n')) ++a;
    while (b > a && (s[b-1] == ' ' || s[b-1] == '\t' || s[b-1] == '\r' || s[b-1] == '\n')) --b;
    return s.substr(a, b - a);
}

std::string lowerCopy(const std::string& s) {
    std::string r = s;
    for (char& c : r) if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    return r;
}

// Kesh kaliti: kichik harf + '/' ajratgich (Windows fayl tizimi registrga sezgir emas).
std::string cacheKey(const std::string& p) { return lowerCopy(toSlash(p)); }

// map_Kd dan keyingi opsiyalarni (-s, -o, -bm ...) tashlab, faqat fayl nomini qaytaradi.
// Nomda bo'shliq va '+' bo'lishi mumkin — shuning uchun qolgan hamma narsa nom hisoblanadi.
std::string stripMapOptions(const std::string& in) {
    size_t i = 0;
    while (i < in.size()) {
        while (i < in.size() && (in[i] == ' ' || in[i] == '\t')) ++i;
        if (i >= in.size() || in[i] != '-') break;
        // opsiya nomini o'tkazib yuboramiz
        while (i < in.size() && in[i] != ' ' && in[i] != '\t') ++i;
        // opsiyaning raqamli / on-off argumentlarini o'tkazib yuboramiz
        for (;;) {
            size_t j = i;
            while (j < in.size() && (in[j] == ' ' || in[j] == '\t')) ++j;
            if (j >= in.size()) { i = j; break; }
            size_t e = j;
            while (e < in.size() && in[e] != ' ' && in[e] != '\t') ++e;
            std::string tok = in.substr(j, e - j);
            char* endp = nullptr;
            std::strtod(tok.c_str(), &endp);
            bool numeric = (endp != nullptr && endp != tok.c_str() && *endp == '\0');
            if (numeric || tok == "on" || tok == "off") { i = e; continue; }
            break;
        }
    }
    return trimBoth(in.substr(i));
}

// ---------------------------------------------------------------------------
//  MTL
// ---------------------------------------------------------------------------

struct MtlDef {
    float       kd[3] = {1.0f, 1.0f, 1.0f};
    float       alpha = 1.0f;
    std::string mapKd;      // xom nom (.mtl da yozilgani)
};

// Tekstura yo'lini .obj papkasiga nisbatan hal qiladi. Topilmasa bo'sh satr.
std::string resolveTexturePath(const std::string& objDir, const std::string& raw) {
    if (raw.empty()) return std::string();
    std::string name = toSlash(trimBoth(raw));
    std::string base = baseOf(name);

    // Ehtimoliy joylashuvlar — birinchi mavjud bo'lgani olinadi.
    std::string cand[6];
    int n = 0;
    cand[n++] = objDir + name;
    cand[n++] = objDir + base;
    cand[n++] = name;                              // mutlaq yoki joriy papkaga nisbatan
    cand[n++] = objDir + "textures/" + base;
    cand[n++] = objDir + "tex/" + base;
    cand[n++] = objDir + "../textures/" + base;
    for (int i = 0; i < n; ++i)
        if (fileExists(cand[i])) return cand[i];
    return std::string();
}

void parseMtl(const std::string& mtlPath, const std::string& objDir,
              std::map<std::string, MtlDef>& out) {
    std::vector<char> buf;
    if (!readWholeFile(mtlPath, buf) || buf.size() < 2) return;

    char* p   = buf.data();
    char* end = buf.data() + buf.size() - 1;   // '\0' dan oldin
    MtlDef*   cur = nullptr;

    while (p < end) {
        char* le = p;
        while (le < end && *le != '\n') ++le;
        // satrni [p, le) tahlil qilamiz
        char* s = p;
        while (s < le && (*s == ' ' || *s == '\t')) ++s;
        size_t len = static_cast<size_t>(le - s);

        if (len >= 7 && std::memcmp(s, "newmtl", 6) == 0 && (s[6] == ' ' || s[6] == '\t')) {
            std::string nm = trimBoth(std::string(s + 6, len - 6));
            if (!nm.empty()) cur = &out[nm];
        } else if (cur && len >= 3 && s[0] == 'K' && s[1] == 'd' && (s[2] == ' ' || s[2] == '\t')) {
            char* q = s + 2;
            for (int i = 0; i < 3; ++i) {
                char* nq = nullptr;
                float v = std::strtof(q, &nq);
                if (nq == q) break;
                cur->kd[i] = v;
                q = nq;
            }
        } else if (cur && len >= 3 && s[0] == 'd' && (s[1] == ' ' || s[1] == '\t')) {
            char* nq = nullptr;
            float v = std::strtof(s + 1, &nq);
            if (nq != s + 1) cur->alpha = clampf(v, 0.0f, 1.0f);
        } else if (cur && len >= 4 && std::memcmp(s, "Tr", 2) == 0 && (s[2] == ' ' || s[2] == '\t')) {
            char* nq = nullptr;
            float v = std::strtof(s + 2, &nq);
            if (nq != s + 2) cur->alpha = clampf(1.0f - v, 0.0f, 1.0f);
        } else if (cur && len >= 7 && std::memcmp(s, "map_Kd", 6) == 0 && (s[6] == ' ' || s[6] == '\t')) {
            cur->mapKd = stripMapOptions(std::string(s + 6, len - 6));
        }
        p = le + 1;
    }
    (void)objDir;   // yo'l keyinroq (submesh qurishda) hal qilinadi
}

// ---------------------------------------------------------------------------
//  Chizish (klient massivlar)
// ---------------------------------------------------------------------------

void drawArrays(const MeshVertex* base, size_t vcount, const std::vector<SubMesh>& subs) {
    if (!base || vcount == 0) return;

    const GLsizei stride = static_cast<GLsizei>(sizeof(MeshVertex));

    // Oldingi holatni eslab qolamiz
    const GLboolean texWas = glIsEnabled(GL_TEXTURE_2D);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    glVertexPointer  (3, GL_FLOAT, stride, &base->px);
    glNormalPointer  (   GL_FLOAT, stride, &base->nx);
    glTexCoordPointer(2, GL_FLOAT, stride, &base->u);

    if (subs.empty()) {
        // Material yo'q — hammasini bitta chaqiruvda
        glDisable(GL_TEXTURE_2D);
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vcount - (vcount % 3)));
    } else {
        for (const SubMesh& sm : subs) {
            if (sm.count == 0 || sm.first >= vcount) continue;
            size_t cnt = sm.count;
            if (sm.first + cnt > vcount) cnt = vcount - sm.first;
            cnt -= (cnt % 3);
            if (cnt == 0) continue;

            if (sm.tex != nullptr && sm.tex->valid()) {
                glEnable(GL_TEXTURE_2D);
                // Tekstura rejimini ANIQ o'rnatamiz. Ilgari bu yerda hech narsa
                // qo'yilmagani uchun relyefning GL_COMBINE holati oqib kelib,
                // teksturali rekvizitlarni (devor, tom, panjara) qora qilib qo'yardi.
                glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
                sm.tex->bind();
            } else {
                glDisable(GL_TEXTURE_2D);
            }
            glColor4f(sm.kd[0], sm.kd[1], sm.kd[2], sm.alpha);
            glDrawArrays(GL_TRIANGLES, static_cast<GLint>(sm.first), static_cast<GLsizei>(cnt));
        }
    }

    // Holatni tiklaymiz
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    if (texWas) glEnable(GL_TEXTURE_2D); else glDisable(GL_TEXTURE_2D);
}

// ---------------------------------------------------------------------------
//  Protsedural primitivlar uchun quruvchilar
// ---------------------------------------------------------------------------

inline void pushV(std::vector<MeshVertex>& o,
                  float px, float py, float pz,
                  float nx, float ny, float nz,
                  float u,  float v) {
    MeshVertex mv;
    mv.px = px; mv.py = py; mv.pz = pz;
    mv.nx = nx; mv.ny = ny; mv.nz = nz;
    mv.u  = u;  mv.v  = v;
    o.push_back(mv);
}

// To'rtburchak (a,b,c,d — CCW tashqaridan qaralganda)
void addQuad(std::vector<MeshVertex>& o,
             const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& d,
             const Vec3& n) {
    pushV(o, a.x,a.y,a.z, n.x,n.y,n.z, 0.0f, 0.0f);
    pushV(o, b.x,b.y,b.z, n.x,n.y,n.z, 1.0f, 0.0f);
    pushV(o, c.x,c.y,c.z, n.x,n.y,n.z, 1.0f, 1.0f);
    pushV(o, a.x,a.y,a.z, n.x,n.y,n.z, 0.0f, 0.0f);
    pushV(o, c.x,c.y,c.z, n.x,n.y,n.z, 1.0f, 1.0f);
    pushV(o, d.x,d.y,d.z, n.x,n.y,n.z, 0.0f, 1.0f);
}

void buildCube(std::vector<MeshVertex>& o) {
    o.clear();
    o.reserve(36);
    const float h = 0.5f;
    // +X
    addQuad(o, {  h,0.0f, h}, {  h,0.0f,-h}, {  h,1.0f,-h}, {  h,1.0f, h}, { 1, 0, 0});
    // -X
    addQuad(o, { -h,0.0f,-h}, { -h,0.0f, h}, { -h,1.0f, h}, { -h,1.0f,-h}, {-1, 0, 0});
    // +Z
    addQuad(o, { -h,0.0f, h}, {  h,0.0f, h}, {  h,1.0f, h}, { -h,1.0f, h}, { 0, 0, 1});
    // -Z
    addQuad(o, {  h,0.0f,-h}, { -h,0.0f,-h}, { -h,1.0f,-h}, {  h,1.0f,-h}, { 0, 0,-1});
    // +Y
    addQuad(o, { -h,1.0f,-h}, { -h,1.0f, h}, {  h,1.0f, h}, {  h,1.0f,-h}, { 0, 1, 0});
    // -Y
    addQuad(o, { -h,0.0f, h}, { -h,0.0f,-h}, {  h,0.0f,-h}, {  h,0.0f, h}, { 0,-1, 0});
}

void buildCylinder(std::vector<MeshVertex>& o, int segs) {
    o.clear();
    if (segs < 3)  segs = 3;
    if (segs > 256) segs = 256;
    o.reserve(static_cast<size_t>(segs) * 12u);
    const float r = 0.5f;
    for (int i = 0; i < segs; ++i) {
        float t0 = static_cast<float>(i)     / static_cast<float>(segs);
        float t1 = static_cast<float>(i + 1) / static_cast<float>(segs);
        float a0 = t0 * TAU, a1 = t1 * TAU;
        float c0 = std::cos(a0), s0 = std::sin(a0);
        float c1 = std::cos(a1), s1 = std::sin(a1);

        // yon devor (ikki uchburchak, har verteksda o'z radial normali)
        pushV(o, r*c0, 0.0f, r*s0,  c0, 0.0f, s0, t0, 0.0f);
        pushV(o, r*c0, 1.0f, r*s0,  c0, 0.0f, s0, t0, 1.0f);
        pushV(o, r*c1, 1.0f, r*s1,  c1, 0.0f, s1, t1, 1.0f);
        pushV(o, r*c0, 0.0f, r*s0,  c0, 0.0f, s0, t0, 0.0f);
        pushV(o, r*c1, 1.0f, r*s1,  c1, 0.0f, s1, t1, 1.0f);
        pushV(o, r*c1, 0.0f, r*s1,  c1, 0.0f, s1, t1, 0.0f);

        // yuqori qopqoq
        pushV(o, 0.0f, 1.0f, 0.0f,  0.0f, 1.0f, 0.0f, 0.5f, 0.5f);
        pushV(o, r*c1, 1.0f, r*s1,  0.0f, 1.0f, 0.0f, 0.5f + 0.5f*c1, 0.5f + 0.5f*s1);
        pushV(o, r*c0, 1.0f, r*s0,  0.0f, 1.0f, 0.0f, 0.5f + 0.5f*c0, 0.5f + 0.5f*s0);

        // pastki qopqoq
        pushV(o, 0.0f, 0.0f, 0.0f,  0.0f,-1.0f, 0.0f, 0.5f, 0.5f);
        pushV(o, r*c0, 0.0f, r*s0,  0.0f,-1.0f, 0.0f, 0.5f + 0.5f*c0, 0.5f + 0.5f*s0);
        pushV(o, r*c1, 0.0f, r*s1,  0.0f,-1.0f, 0.0f, 0.5f + 0.5f*c1, 0.5f + 0.5f*s1);
    }
}

void buildCone(std::vector<MeshVertex>& o, int segs) {
    o.clear();
    if (segs < 3)  segs = 3;
    if (segs > 256) segs = 256;
    o.reserve(static_cast<size_t>(segs) * 6u);
    const float r = 0.5f;
    const float invLen = 1.0f / std::sqrt(1.0f + r * r);   // (cos, r, sin) normalizatsiyasi
    for (int i = 0; i < segs; ++i) {
        float t0 = static_cast<float>(i)     / static_cast<float>(segs);
        float t1 = static_cast<float>(i + 1) / static_cast<float>(segs);
        float a0 = t0 * TAU, a1 = t1 * TAU, am = 0.5f * (a0 + a1);
        float c0 = std::cos(a0), s0 = std::sin(a0);
        float c1 = std::cos(a1), s1 = std::sin(a1);
        float cm = std::cos(am), sm = std::sin(am);

        // yon yuza
        pushV(o, r*c0, 0.0f, r*s0,  c0*invLen, r*invLen, s0*invLen, t0, 0.0f);
        pushV(o, 0.0f, 1.0f, 0.0f,  cm*invLen, r*invLen, sm*invLen, 0.5f*(t0+t1), 1.0f);
        pushV(o, r*c1, 0.0f, r*s1,  c1*invLen, r*invLen, s1*invLen, t1, 0.0f);

        // asos
        pushV(o, 0.0f, 0.0f, 0.0f,  0.0f,-1.0f, 0.0f, 0.5f, 0.5f);
        pushV(o, r*c0, 0.0f, r*s0,  0.0f,-1.0f, 0.0f, 0.5f + 0.5f*c0, 0.5f + 0.5f*s0);
        pushV(o, r*c1, 0.0f, r*s1,  0.0f,-1.0f, 0.0f, 0.5f + 0.5f*c1, 0.5f + 0.5f*s1);
    }
}

void buildSphere(std::vector<MeshVertex>& o, int rings, int segs) {
    o.clear();
    if (rings < 3)  rings = 3;
    if (rings > 128) rings = 128;
    if (segs  < 3)  segs  = 3;
    if (segs  > 256) segs  = 256;
    o.reserve(static_cast<size_t>(rings) * static_cast<size_t>(segs) * 6u);
    const float R  = 0.5f;
    const float cy = 0.5f;   // markaz: y 0..1 oralig'ida bo'lsin
    for (int i = 0; i < rings; ++i) {
        float th0 = PI * static_cast<float>(i)     / static_cast<float>(rings);
        float th1 = PI * static_cast<float>(i + 1) / static_cast<float>(rings);
        for (int j = 0; j < segs; ++j) {
            float ph0 = TAU * static_cast<float>(j)     / static_cast<float>(segs);
            float ph1 = TAU * static_cast<float>(j + 1) / static_cast<float>(segs);

            float n[4][3];
            float uv[4][2];
            const float th[4] = { th0, th0, th1, th1 };
            const float ph[4] = { ph0, ph1, ph1, ph0 };
            const float uu[4] = { static_cast<float>(j)   / static_cast<float>(segs),
                                  static_cast<float>(j+1) / static_cast<float>(segs),
                                  static_cast<float>(j+1) / static_cast<float>(segs),
                                  static_cast<float>(j)   / static_cast<float>(segs) };
            const float vv[4] = { 1.0f - static_cast<float>(i)   / static_cast<float>(rings),
                                  1.0f - static_cast<float>(i)   / static_cast<float>(rings),
                                  1.0f - static_cast<float>(i+1) / static_cast<float>(rings),
                                  1.0f - static_cast<float>(i+1) / static_cast<float>(rings) };
            for (int k = 0; k < 4; ++k) {
                float st = std::sin(th[k]);
                n[k][0] = st * std::cos(ph[k]);
                n[k][1] = std::cos(th[k]);
                n[k][2] = st * std::sin(ph[k]);
                uv[k][0] = uu[k];
                uv[k][1] = vv[k];
            }
            const int order[6] = {0, 1, 2, 0, 2, 3};
            for (int k = 0; k < 6; ++k) {
                int q = order[k];
                pushV(o, n[q][0]*R, cy + n[q][1]*R, n[q][2]*R,
                         n[q][0], n[q][1], n[q][2], uv[q][0], uv[q][1]);
            }
        }
    }
}

void buildQuadXZ(std::vector<MeshVertex>& o) {
    o.clear();
    o.reserve(6);
    const float h = 0.5f;
    pushV(o, -h, 0.0f,  h, 0,1,0, 0.0f, 0.0f);
    pushV(o,  h, 0.0f,  h, 0,1,0, 1.0f, 0.0f);
    pushV(o,  h, 0.0f, -h, 0,1,0, 1.0f, 1.0f);
    pushV(o, -h, 0.0f,  h, 0,1,0, 0.0f, 0.0f);
    pushV(o,  h, 0.0f, -h, 0,1,0, 1.0f, 1.0f);
    pushV(o, -h, 0.0f, -h, 0,1,0, 0.0f, 1.0f);
}

// Primitivning submesh va bbox ma'lumotlarini to'ldiradi.
void finishPrim(const std::vector<MeshVertex>& v, std::vector<SubMesh>& subs,
                Vec3& bbMin, Vec3& bbMax) {
    subs.clear();
    if (v.empty()) { bbMin = bbMax = Vec3{0,0,0}; return; }
    SubMesh sm;
    sm.first = 0;
    sm.count = v.size();
    sm.material = "default";
    sm.tex = nullptr;
    sm.kd[0] = sm.kd[1] = sm.kd[2] = 1.0f;
    sm.alpha = 1.0f;
    subs.push_back(sm);

    bbMin = Vec3{ v[0].px, v[0].py, v[0].pz };
    bbMax = bbMin;
    for (const MeshVertex& mv : v) {
        bbMin.x = std::min(bbMin.x, mv.px);
        bbMin.y = std::min(bbMin.y, mv.py);
        bbMin.z = std::min(bbMin.z, mv.pz);
        bbMax.x = std::max(bbMax.x, mv.px);
        bbMax.y = std::max(bbMax.y, mv.py);
        bbMax.z = std::max(bbMax.z, mv.pz);
    }
}

// ---------------------------------------------------------------------------
//  Kesh (fayl modellari + protsedural primitivlar)
// ---------------------------------------------------------------------------

std::map<std::string, Mesh*>& fileCache() {
    static std::map<std::string, Mesh*> m;
    return m;
}
std::map<std::string, Mesh*>& primCache() {
    static std::map<std::string, Mesh*> m;
    return m;
}

} // anonim namespace

// ---------------------------------------------------------------------------
//  Mesh
// ---------------------------------------------------------------------------

Mesh::~Mesh() {
    destroy();
}

void Mesh::destroy() {
    if (list_ != 0) {
        // GL konteksti yo'q bo'lsa chaqirmaymiz (dastur yopilishida halokat bo'lmasin)
        if (::wglGetCurrentContext() != nullptr) glDeleteLists(list_, 1);
        list_ = 0;
    }
    vertices_.clear();
    vertices_.shrink_to_fit();
    subs_.clear();
    bbMin_ = bbMax_ = Vec3{0,0,0};
    path_.clear();
}

bool Mesh::load(const std::string& objPath, std::string* err) {
    destroy();
    path_ = objPath;

    std::vector<char> buf;
    if (!readWholeFile(objPath, buf) || buf.size() < 2) {
        if (err) *err = "OBJ faylni ochib bo'lmadi: " + objPath;
        return false;
    }

    const std::string objDir = dirOf(objPath);

    // --- xom ma'lumotlar ---
    std::vector<Vec3> pos;   pos.reserve(1u << 16);
    std::vector<Vec2> uvs;   uvs.reserve(1u << 16);
    std::vector<Vec3> nrm;   nrm.reserve(1u << 16);

    std::map<std::string, MtlDef> mtls;

    // Emissiya paytidagi holat
    std::string curMtl;                 // joriy usemtl nomi
    std::vector<int>  vertPos;          // har emissiya qilingan verteksning pozitsiya indeksi
    std::vector<unsigned char> needSm;  // shu verteksga silliq normal kerakmi
    std::vector<Vec3> accN;             // pozitsiya bo'yicha to'plangan normallar
    bool anyMissingNormal = false;

    vertices_.reserve(1u << 17);
    vertPos.reserve(1u << 17);
    needSm.reserve(1u << 17);

    struct FaceRef { long v, t, n; };
    std::vector<FaceRef> face;
    face.reserve(64);

    char* p   = buf.data();
    char* end = buf.data() + buf.size() - 1;   // '\0' dan oldin

    auto beginSub = [&](const std::string& name) {
        if (!subs_.empty()) {
            SubMesh& last = subs_.back();
            if (last.material == name) return;                 // bir xil material — birlashtiramiz
            last.count = vertices_.size() - last.first;
            if (last.count == 0) { last.material = name; return; }   // bo'sh submesh — nomini almashtiramiz
        }
        SubMesh sm;
        sm.first    = vertices_.size();
        sm.count    = 0;
        sm.material = name;
        subs_.push_back(sm);
    };

    while (p < end) {
        char* le = p;
        while (le < end && *le != '\n') ++le;

        char* s = p;
        while (s < le && (*s == ' ' || *s == '\t')) ++s;
        const size_t len = static_cast<size_t>(le - s);
        p = le + 1;
        if (len == 0 || *s == '#') continue;

        // ---- v / vt / vn ----
        if (s[0] == 'v' && len >= 2) {
            if (s[1] == ' ' || s[1] == '\t') {
                char* q = s + 1; char* nq = nullptr;
                float x = std::strtof(q, &nq); if (nq == q) continue; q = nq;
                float y = std::strtof(q, &nq); if (nq == q) continue; q = nq;
                float z = std::strtof(q, &nq); if (nq == q) continue;
                pos.push_back(Vec3{x, y, z});
                continue;
            }
            if (s[1] == 't' && len >= 3) {
                char* q = s + 2; char* nq = nullptr;
                float u = std::strtof(q, &nq); if (nq == q) { uvs.push_back(Vec2{0,0}); continue; } q = nq;
                float v = std::strtof(q, &nq); if (nq == q) v = 0.0f;
                uvs.push_back(Vec2{u, v});
                continue;
            }
            if (s[1] == 'n' && len >= 3) {
                char* q = s + 2; char* nq = nullptr;
                float x = std::strtof(q, &nq); if (nq == q) { nrm.push_back(Vec3{0,1,0}); continue; } q = nq;
                float y = std::strtof(q, &nq); if (nq == q) y = 0.0f; q = nq;
                float z = std::strtof(q, &nq); if (nq == q) z = 0.0f;
                // Ba'zi eksportchilar birlik bo'lmagan normal yozadi (masalan uzunligi 0.1) —
                // bu OpenGL yoritishini buzadi, shuning uchun darhol normallashtiramiz.
                Vec3 n{x, y, z};
                const float nl = length(n);
                nrm.push_back(nl > 1e-12f ? (n / nl) : Vec3{0, 1, 0});
                continue;
            }
            continue;
        }

        // ---- f ----
        if (s[0] == 'f' && len >= 2 && (s[1] == ' ' || s[1] == '\t')) {
            face.clear();
            char* q = s + 1;
            while (q < le) {
                while (q < le && (*q == ' ' || *q == '\t' || *q == '\r')) ++q;
                if (q >= le) break;
                char* nq = nullptr;
                long vi = std::strtol(q, &nq, 10);
                if (nq == q) { while (q < le && *q != ' ' && *q != '\t') ++q; continue; }
                q = nq;
                long ti = 0, ni = 0;
                if (q < le && *q == '/') {
                    ++q;
                    if (q < le && *q != '/') { ti = std::strtol(q, &nq, 10); if (nq != q) q = nq; }
                    if (q < le && *q == '/') {
                        ++q;
                        ni = std::strtol(q, &nq, 10);
                        if (nq != q) q = nq;
                    }
                }
                FaceRef fr; fr.v = vi; fr.t = ti; fr.n = ni;
                face.push_back(fr);
                if (face.size() > 4096u) break;   // buzilgan fayldan himoya
            }
            if (face.size() < 3) continue;

            // manfiy (nisbiy) indekslarni hal qilamiz
            const long np = static_cast<long>(pos.size());
            const long nt = static_cast<long>(uvs.size());
            const long nn = static_cast<long>(nrm.size());
            bool bad = false;
            for (FaceRef& fr : face) {
                fr.v = (fr.v > 0) ? (fr.v - 1) : ((fr.v < 0) ? (np + fr.v) : -1);
                if (fr.v < 0 || fr.v >= np) { bad = true; break; }
                fr.t = (fr.t > 0) ? (fr.t - 1) : ((fr.t < 0) ? (nt + fr.t) : -1);
                if (fr.t < 0 || fr.t >= nt) fr.t = -1;
                fr.n = (fr.n > 0) ? (fr.n - 1) : ((fr.n < 0) ? (nn + fr.n) : -1);
                if (fr.n < 0 || fr.n >= nn) fr.n = -1;
            }
            if (bad) continue;   // indeks chegaradan chiqdi — yuzni tashlab ketamiz

            if (subs_.empty()) beginSub(curMtl.empty() ? std::string("default") : curMtl);

            // FAN triangulyatsiya: (v0, vi, vi+1)
            const size_t nv = face.size();
            for (size_t i = 1; i + 1 < nv; ++i) {
                const FaceRef* tri[3] = { &face[0], &face[i], &face[i + 1] };

                // yuz normali (vn yo'q bo'lsa kerak bo'ladi)
                const Vec3& a = pos[static_cast<size_t>(tri[0]->v)];
                const Vec3& b = pos[static_cast<size_t>(tri[1]->v)];
                const Vec3& c = pos[static_cast<size_t>(tri[2]->v)];
                Vec3 fn = cross(b - a, c - a);
                float fl = length(fn);
                if (fl > 1e-20f) fn = fn / fl; else fn = Vec3{0, 1, 0};

                for (int k = 0; k < 3; ++k) {
                    const FaceRef& fr = *tri[k];
                    const Vec3& pv = pos[static_cast<size_t>(fr.v)];
                    MeshVertex mv;
                    mv.px = pv.x; mv.py = pv.y; mv.pz = pv.z;
                    if (fr.t >= 0) { mv.u = uvs[static_cast<size_t>(fr.t)].x; mv.v = uvs[static_cast<size_t>(fr.t)].y; }
                    else           { mv.u = 0.0f; mv.v = 0.0f; }

                    bool sm = (fr.n < 0);
                    if (!sm) {
                        const Vec3& nv3 = nrm[static_cast<size_t>(fr.n)];
                        mv.nx = nv3.x; mv.ny = nv3.y; mv.nz = nv3.z;
                    } else {
                        mv.nx = fn.x; mv.ny = fn.y; mv.nz = fn.z;   // vaqtinchalik
                        anyMissingNormal = true;
                        if (accN.size() < pos.size()) accN.resize(pos.size(), Vec3{0,0,0});
                        accN[static_cast<size_t>(fr.v)] += fn;
                    }
                    vertices_.push_back(mv);
                    vertPos.push_back(static_cast<int>(fr.v));
                    needSm.push_back(sm ? 1u : 0u);
                }
            }
            continue;
        }

        // ---- usemtl ----
        if (len >= 7 && std::memcmp(s, "usemtl", 6) == 0 && (s[6] == ' ' || s[6] == '\t' || s[6] == '\r')) {
            std::string nm = trimBoth(std::string(s + 6, len - 6));
            if (nm.empty()) nm = "default";
            curMtl = nm;
            beginSub(nm);
            continue;
        }

        // ---- mtllib ----
        if (len >= 7 && std::memcmp(s, "mtllib", 6) == 0 && (s[6] == ' ' || s[6] == '\t')) {
            std::string nm = trimBoth(std::string(s + 6, len - 6));
            if (!nm.empty()) {
                std::string mp = objDir + toSlash(nm);
                if (!fileExists(mp)) mp = objDir + baseOf(toSlash(nm));
                if (!fileExists(mp)) mp = toSlash(nm);
                if (fileExists(mp)) parseMtl(mp, objDir, mtls);
            }
            continue;
        }

        // ---- o / g (guruhlar) ----
        // Material bo'yicha guruhlash yetarli, shuning uchun faqat e'tiborsiz qoldiramiz.
        if ((s[0] == 'o' || s[0] == 'g') && (len == 1 || s[1] == ' ' || s[1] == '\t' || s[1] == '\r')) {
            continue;
        }
    }

    // oxirgi submeshni yopamiz
    if (!subs_.empty()) {
        SubMesh& last = subs_.back();
        last.count = vertices_.size() - last.first;
    }
    // bo'sh submeshlarni olib tashlaymiz
    {
        std::vector<SubMesh> keep;
        keep.reserve(subs_.size());
        for (const SubMesh& sm : subs_) if (sm.count > 0) keep.push_back(sm);
        subs_.swap(keep);
    }

    if (vertices_.empty()) {
        if (err) *err = "OBJ ichida uchburchak topilmadi: " + objPath;
        destroy();
        path_ = objPath;
        return false;
    }

    // --- silliq normallar (vn bo'lmagan vertekslar uchun) ---
    if (anyMissingNormal && !accN.empty()) {
        for (Vec3& n : accN) {
            float l = length(n);
            n = (l > 1e-20f) ? (n / l) : Vec3{0, 1, 0};
        }
        const size_t nvz = vertices_.size();
        for (size_t i = 0; i < nvz; ++i) {
            if (!needSm[i]) continue;
            size_t pi = static_cast<size_t>(vertPos[i]);
            if (pi >= accN.size()) continue;
            vertices_[i].nx = accN[pi].x;
            vertices_[i].ny = accN[pi].y;
            vertices_[i].nz = accN[pi].z;
        }
    }

    // --- materiallarni submeshlarga bog'laymiz ---
    // Bir xil tekstura yo'li bir marta yuklanadi (Texture::get o'zi ham keshlaydi).
    for (SubMesh& sm : subs_) {
        std::map<std::string, MtlDef>::const_iterator it = mtls.find(sm.material);
        if (it == mtls.end()) continue;
        const MtlDef& md = it->second;
        sm.kd[0] = md.kd[0]; sm.kd[1] = md.kd[1]; sm.kd[2] = md.kd[2];
        sm.alpha = md.alpha;
        if (!md.mapKd.empty()) {
            std::string tp = resolveTexturePath(objDir, md.mapKd);
            if (!tp.empty()) sm.tex = Texture::get(tp, 1024);   // fayl topilmasa — nullptr qoladi
        }
    }

    // --- chegaraviy quti ---
    bbMin_ = Vec3{ vertices_[0].px, vertices_[0].py, vertices_[0].pz };
    bbMax_ = bbMin_;
    for (const MeshVertex& mv : vertices_) {
        bbMin_.x = std::min(bbMin_.x, mv.px);
        bbMin_.y = std::min(bbMin_.y, mv.py);
        bbMin_.z = std::min(bbMin_.z, mv.pz);
        bbMax_.x = std::max(bbMax_.x, mv.px);
        bbMax_.y = std::max(bbMax_.y, mv.py);
        bbMax_.z = std::max(bbMax_.z, mv.pz);
    }

    if (err) err->clear();
    return true;
}

void Mesh::draw() const {
    if (vertices_.empty()) return;
    drawArrays(vertices_.data(), vertices_.size(), subs_);
}

void Mesh::drawDeformed(const std::vector<MeshVertex>& deformed) const {
    if (vertices_.empty()) return;
    if (deformed.size() != vertices_.size()) { draw(); return; }   // xavfsiz zaxira yo'l
    drawArrays(deformed.data(), deformed.size(), subs_);
}

void Mesh::buildDisplayList() {
    if (vertices_.empty()) return;
    if (::wglGetCurrentContext() == nullptr) return;
    if (list_ != 0) { glDeleteLists(list_, 1); list_ = 0; }
    GLuint id = glGenLists(1);
    if (id == 0) return;
    glNewList(id, GL_COMPILE);
    draw();
    glEndList();
    list_ = id;
}

void Mesh::drawList() const {
    if (list_ != 0) glCallList(list_);
    else            draw();
}

// ---------------------------------------------------------------------------
//  Kesh
// ---------------------------------------------------------------------------

Mesh* Mesh::get(const std::string& objPath) {
    if (objPath.empty()) return nullptr;
    const std::string key = cacheKey(objPath);
    std::map<std::string, Mesh*>& c = fileCache();
    std::map<std::string, Mesh*>::iterator it = c.find(key);
    if (it != c.end()) return it->second;      // nullptr ham bo'lishi mumkin (avval yuklanmagan)

    Mesh* m = new (std::nothrow) Mesh();
    if (!m) { c[key] = nullptr; return nullptr; }
    std::string err;
    if (!m->load(objPath, &err)) {
        delete m;
        c[key] = nullptr;                       // qayta-qayta urinmaslik uchun eslab qolamiz
        return nullptr;
    }
    c[key] = m;
    return m;
}

// Bola nisbatlari: bo'y bo'yicha uch mintaqa alohida masshtablanadi.
// Balandlik normallashtirilgan (0 = oyoq, 1 = bosh tepasi).
static void deformChild(std::vector<MeshVertex>& v, const Vec3& mn, const Vec3& mx) {
    const float h = std::max(1e-6f, mx.y - mn.y);
    const float cx = (mn.x + mx.x) * 0.5f, cz = (mn.z + mx.z) * 0.5f;
    const float kLeg = 0.72f, kTorso = 0.90f, kHead = 1.28f, kWide = 0.92f;
    // mintaqalar: oyoq 0..0.50, tana 0.50..0.86, bosh 0.86..1
    const float legTop   = mn.y + h * 0.50f * kLeg;
    const float torsoTop = legTop + h * 0.36f * kTorso;
    const float headMidN = 0.93f;                                   // bosh markazi
    for (size_t i = 0; i < v.size(); ++i) {
        MeshVertex& p = v[i];
        const float yn = (p.py - mn.y) / h;
        float ny;
        if (yn < 0.50f)      ny = mn.y + (p.py - mn.y) * kLeg;
        else if (yn < 0.86f) ny = legTop + (p.py - (mn.y + h * 0.50f)) * kTorso;
        else {
            // bosh: markaz atrofida hamma o'qda kattalashadi
            const float headMid = mn.y + h * headMidN;
            ny = torsoTop + (p.py - (mn.y + h * 0.86f)) * kHead * 0.8f;
            const float dy = (p.py - headMid);
            (void)dy;
            p.px = cx + (p.px - cx) * kHead;
            p.pz = cz + (p.pz - cz) * kHead;
            p.py = ny;
            continue;
        }
        p.py = ny;
        p.px = cx + (p.px - cx) * kWide;
        p.pz = cz + (p.pz - cz) * kWide;
    }
}

Mesh* Mesh::getVariant(const std::string& objPath, const std::string& variant) {
    if (variant.empty()) return get(objPath);
    const std::string key = cacheKey(objPath) + "#" + variant;
    std::map<std::string, Mesh*>& c = fileCache();
    std::map<std::string, Mesh*>::iterator it = c.find(key);
    if (it != c.end()) return it->second;
    Mesh* base = get(objPath);
    if (!base) { c[key] = nullptr; return nullptr; }

    Mesh* m = new (std::nothrow) Mesh();
    if (!m) { c[key] = nullptr; return nullptr; }
    m->vertices_ = base->vertices_;
    m->subs_     = base->subs_;            // tekstura ko'rsatkichlari keshga tegishli
    m->path_     = base->path_ + "#" + variant;
    if (variant == "child" || variant == "bola") deformChild(m->vertices_, base->bbMin_, base->bbMax_);
    // chegara qutisini qayta hisoblaymiz (rigging shunga tayanadi)
    Vec3 mn{1e9f, 1e9f, 1e9f}, mx{-1e9f, -1e9f, -1e9f};
    for (size_t i = 0; i < m->vertices_.size(); ++i) {
        const MeshVertex& p = m->vertices_[i];
        mn.x = std::min(mn.x, p.px); mn.y = std::min(mn.y, p.py); mn.z = std::min(mn.z, p.pz);
        mx.x = std::max(mx.x, p.px); mx.y = std::max(mx.y, p.py); mx.z = std::max(mx.z, p.pz);
    }
    m->bbMin_ = mn; m->bbMax_ = mx;
    c[key] = m;
    return m;
}

void Mesh::clearCache() {
    std::map<std::string, Mesh*>& c = fileCache();
    for (std::map<std::string, Mesh*>::iterator it = c.begin(); it != c.end(); ++it)
        delete it->second;
    c.clear();

    std::map<std::string, Mesh*>& pc = primCache();
    for (std::map<std::string, Mesh*>::iterator it = pc.begin(); it != pc.end(); ++it)
        delete it->second;
    pc.clear();
}

// ---------------------------------------------------------------------------
//  Protsedural primitivlar (har biri bir marta quriladi va keshlanadi)
// ---------------------------------------------------------------------------

Mesh* Mesh::unitCube() {
    Mesh*& slot = primCache()["#cube"];
    if (slot) return slot;
    Mesh* m = new (std::nothrow) Mesh();
    if (!m) return nullptr;
    buildCube(m->vertices_);
    finishPrim(m->vertices_, m->subs_, m->bbMin_, m->bbMax_);
    m->path_ = "#cube";
    slot = m;
    return m;
}

Mesh* Mesh::unitCylinder(int segments) {
    if (segments < 3)  segments = 3;
    if (segments > 256) segments = 256;
    char key[32];
    std::snprintf(key, sizeof(key), "#cyl:%d", segments);
    Mesh*& slot = primCache()[key];
    if (slot) return slot;
    Mesh* m = new (std::nothrow) Mesh();
    if (!m) return nullptr;
    buildCylinder(m->vertices_, segments);
    finishPrim(m->vertices_, m->subs_, m->bbMin_, m->bbMax_);
    m->path_ = key;
    slot = m;
    return m;
}

Mesh* Mesh::unitCone(int segments) {
    if (segments < 3)  segments = 3;
    if (segments > 256) segments = 256;
    char key[32];
    std::snprintf(key, sizeof(key), "#cone:%d", segments);
    Mesh*& slot = primCache()[key];
    if (slot) return slot;
    Mesh* m = new (std::nothrow) Mesh();
    if (!m) return nullptr;
    buildCone(m->vertices_, segments);
    finishPrim(m->vertices_, m->subs_, m->bbMin_, m->bbMax_);
    m->path_ = key;
    slot = m;
    return m;
}

Mesh* Mesh::unitSphere(int rings, int segs) {
    if (rings < 3)  rings = 3;
    if (rings > 128) rings = 128;
    if (segs  < 3)  segs  = 3;
    if (segs  > 256) segs  = 256;
    char key[40];
    std::snprintf(key, sizeof(key), "#sph:%dx%d", rings, segs);
    Mesh*& slot = primCache()[key];
    if (slot) return slot;
    Mesh* m = new (std::nothrow) Mesh();
    if (!m) return nullptr;
    buildSphere(m->vertices_, rings, segs);
    finishPrim(m->vertices_, m->subs_, m->bbMin_, m->bbMax_);
    m->path_ = key;
    slot = m;
    return m;
}

Mesh* Mesh::quadXZ() {
    Mesh*& slot = primCache()["#quadxz"];
    if (slot) return slot;
    Mesh* m = new (std::nothrow) Mesh();
    if (!m) return nullptr;
    buildQuadXZ(m->vertices_);
    finishPrim(m->vertices_, m->subs_, m->bbMin_, m->bbMax_);
    m->path_ = "#quadxz";
    slot = m;
    return m;
}

} // namespace ert
