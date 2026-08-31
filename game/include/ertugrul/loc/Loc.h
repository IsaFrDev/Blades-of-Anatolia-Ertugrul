#pragma once
// UTF-8 lokalizatsiya: keys,uz,tr,en ustunli CSV.
#include <string>
#include <vector>

namespace ert {

class Loc {
public:
    static Loc& get();

    // Bir nechta CSV yuklash mumkin (keyingisi oldingisini to'ldiradi/almashtiradi).
    bool loadCsv(const std::string& path);
    void clear();

    void setLanguage(const std::string& code);      // "uz" | "tr" | "en"
    const std::string& language() const;
    static const char* languageNativeName(const std::string& code);   // "O'zbekcha" / "Türkçe" / "English"

    // Kalit topilmasa kalitning o'zini qaytaradi va missing ro'yxatiga qo'shadi.
    const std::string& tr(const std::string& key) const;
    // Kalit topilmasa fallback qaytaradi (missing ro'yxatiga qo'shmaydi).
    std::string trOr(const std::string& key, const std::string& fallback) const;
    bool has(const std::string& key) const;

    size_t keyCount() const;
    // Diagnostika: so'ralgan, lekin topilmagan kalitlar
    const std::vector<std::string>& missingKeys() const;
    void dumpMissing(const std::string& outPath) const;

    static const char* const kLanguages[];    // {"uz","tr","en",nullptr}

private:
    Loc() = default;
};

// Qisqartma
const std::string& T(const std::string& key);

} // namespace ert
