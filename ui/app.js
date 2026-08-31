// UI Navigation Logic
function navigateTo(screenId) {
    // Barcha ekranlarni yashirish
    document.querySelectorAll('.screen').forEach(screen => {
        screen.classList.remove('active');
        screen.classList.add('hidden');
    });

    // Tanlangan ekranni ko'rsatish
    const activeScreen = document.getElementById(screenId);
    if (activeScreen) {
        activeScreen.classList.remove('hidden');
        // Kichik kechikish bilan animatsiya qo'shish
        setTimeout(() => {
            activeScreen.classList.add('active');
        }, 50);
    }

    // Agar HUD tanlansa, orqa fonni to'liq shaffof qilish (o'yin ustiga tushishi uchun)
    const bgLayer = document.querySelector('.bg-layer');
    if (screenId === 'hud') {
        bgLayer.style.opacity = '0';
    } else {
        bgLayer.style.opacity = '1';
    }
}

// Settings Tab Logic
function switchTab(tabId) {
    // Barcha tab tugmalarini nofaol qilish
    document.querySelectorAll('.tab-btn').forEach(btn => {
        btn.classList.remove('active');
    });

    // Bosilgan tugmani faol qilish
    event.target.classList.add('active');

    // Barcha kontentlarni yashirish
    document.querySelectorAll('.tab-content').forEach(content => {
        content.classList.add('hidden');
    });

    // Tanlangan kontentni ko'rsatish
    const activeTab = document.getElementById(tabId + '-tab');
    if (activeTab) {
        activeTab.classList.remove('hidden');
    }
}

// Exit tugmasi (faqat brauzer yoki WebKit integratsiyasi uchun ishlaydi)
document.querySelector('.nav-btn.exit').addEventListener('click', () => {
    console.log("O'yindan chiqish...");
    alert("O'yindan chiqilmoqda (C++ backendga signal yuborildi)");
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
