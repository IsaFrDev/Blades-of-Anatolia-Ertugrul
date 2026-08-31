const translations = {
    uz: {
        subtitle: "Diriliş",
        play: "O'yinni Boshlash",
        settings: "Sozlamalar",
        exit: "Chiqish",
        back: "← Qaytish",
        charTitle: "QAHRAMON TANLASH",
        heroes: "Alplar",
        health: "Sog'lik:",
        armor: "Zirh:",
        weapon: "Qurol:",
        startBtn: "JANGGA KIRISH (PLAY)",
        settingsTitle: "SOZLAMALAR",
        tabGraphics: "Grafika",
        tabGame: "O'yin",
        tabAudio: "Ovoz"
    },
    en: {
        subtitle: "Resurrection",
        play: "Start Game",
        settings: "Settings",
        exit: "Exit",
        back: "← Back",
        charTitle: "SELECT CHARACTER",
        heroes: "Warriors",
        health: "Health:",
        armor: "Armor:",
        weapon: "Weapon:",
        startBtn: "ENTER BATTLE (PLAY)",
        settingsTitle: "SETTINGS",
        tabGraphics: "Graphics",
        tabGame: "Game",
        tabAudio: "Audio"
    },
    tr: {
        subtitle: "Diriliş",
        play: "Oyuna Başla",
        settings: "Ayarlar",
        exit: "Çıkış",
        back: "← Geri Dön",
        charTitle: "KARAKTER SEÇİMİ",
        heroes: "Alpler",
        health: "Sağlık:",
        armor: "Zırh:",
        weapon: "Silah:",
        startBtn: "SAVAŞA GİR (PLAY)",
        settingsTitle: "AYARLAR",
        tabGraphics: "Grafik",
        tabGame: "Oyun",
        tabAudio: "Ses"
    }
};

let currentLang = 'uz';

function changeLanguage(lang) {
    currentLang = lang;
    
    // Update active button
    document.querySelectorAll('.language-selector button').forEach(btn => {
        btn.classList.remove('active');
    });
    event.target.classList.add('active');

    // Apply translations
    const t = translations[lang];
    document.getElementById('t-subtitle').innerText = t.subtitle;
    document.getElementById('t-play').innerText = t.play;
    document.getElementById('t-settings').innerText = t.settings;
    document.getElementById('t-exit').innerText = t.exit;
    document.getElementById('t-back').innerText = t.back;
    document.getElementById('t-back2').innerText = t.back;
    document.getElementById('t-char-title').innerText = t.charTitle;
    document.getElementById('t-heroes').innerText = t.heroes;
    document.getElementById('t-health').innerText = t.health;
    document.getElementById('t-armor').innerText = t.armor;
    document.getElementById('t-weapon').innerText = t.weapon;
    document.getElementById('t-start-btn').innerText = t.startBtn;
    document.getElementById('t-settings-title').innerText = t.settingsTitle;
    document.getElementById('t-tab-graphics').innerText = t.tabGraphics;
    document.getElementById('t-tab-game').innerText = t.tabGame;
    document.getElementById('t-tab-audio').innerText = t.tabAudio;
}

const heroesData = {
    ertugrul: { armor: "Og'ir Charm / Heavy Leather", weapon: "Kayi Qilichi / Kayi Sword", name: "ERTUĞRUL BEY" },
    turgut: { armor: "O'rta Teri / Medium Hide", weapon: "Jangovar Bolta / Battle Axe", name: "TURGUT ALP" },
    bamsi: { armor: "Og'ir Zirh / Heavy Armor", weapon: "Qo'sh Qilich / Dual Swords", name: "BAMSI BEYREK" },
    dogan: { kamon: "Yengil / Light", weapon: "Kamon va Xanjar / Bow & Dagger", name: "DOG'ON ALP" }
};

let currentHero = 'ertugrul';

function selectHero(heroId, element) {
    // Update active state in list
    document.querySelectorAll('.episode-list li').forEach(li => li.classList.remove('active'));
    element.classList.add('active');

    currentHero = heroId;
    const data = heroesData[heroId];
    
    document.getElementById('armor-val').innerText = data.armor || "Yengil";
    document.getElementById('weapon-val').innerText = data.weapon;
    document.getElementById('hud-name').innerText = data.name;
    document.getElementById('hud-weapon').innerText = data.weapon.split('/')[0];
    
    // Animate hologram flash
    const holo = document.getElementById('hero-model-preview');
    holo.style.opacity = '0';
    setTimeout(() => {
        holo.style.opacity = '1';
    }, 200);
}

function navigateTo(screenId) {
    document.querySelectorAll('.screen').forEach(screen => {
        screen.classList.remove('active');
        screen.classList.add('hidden');
    });

    const activeScreen = document.getElementById(screenId);
    if (activeScreen) {
        activeScreen.classList.remove('hidden');
        setTimeout(() => {
            activeScreen.classList.add('active');
        }, 50);
    }
}

function switchTab(tabId) {
    document.querySelectorAll('.tab-btn').forEach(btn => btn.classList.remove('active'));
    event.target.classList.add('active');
    
    document.querySelectorAll('.tab-content').forEach(content => content.classList.add('hidden'));
    const activeTab = document.getElementById(tabId + '-tab');
    if (activeTab) {
        activeTab.classList.remove('hidden');
    }
}

function startGame() {
    console.log(`Starting game with hero: ${currentHero}`);
    alert(`O'yin ${currentHero} bilan boshlanmoqda... (Backend orqali ertugrul.exe ishga tushishi kerak)`);
    navigateTo('hud');
}

// Exit tugmasi
document.getElementById('t-exit').addEventListener('click', () => {
    alert("Backend server to'xtatildi va o'yindan chiqilmoqda.");
});

// ESC tugmasini bosganda HUD dan menyuga qaytish
document.addEventListener('keydown', (e) => {
    if (e.key === 'Escape') {
        const hud = document.getElementById('hud');
        if (hud.classList.contains('active')) {
            navigateTo('main-menu');
        }
    }
});
