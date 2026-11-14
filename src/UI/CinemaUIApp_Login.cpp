#include "../../include/UI/CinemaUIApp.h"
#include "../../include/ActivityLog.h"
#include <sqlite3.h>
#include <algorithm>

namespace UI {
void CinemaUIApp::setupLoginView() {
    float cardW = 1020.f;
    float cardH = 560.f;
    float cardX = (1728.f - cardW) / 2.0f; // 354.f
    float cardY = (972.f - cardH) / 2.0f;  // 206.f
    float leftW = 420.f;
    float rightW = 520.f;
    float rightX = cardX + leftW + (cardW - leftW - rightW) / 2.0f; // Centered horizontally in right panel (814.f)

    loginInputUser = std::unique_ptr<UIInputBox>(new UIInputBox(
        sf::Vector2f(rightX, cardY + 196.f),
        sf::Vector2f(rightW, 54.f),
        "Nhập mã nhân viên hoặc email...",
        16
    ));
    loginInputUser->setTextInset(48.f);
    loginInputUser->setCornerRadius(10.f);
    loginInputUser->setColors(
        sf::Color(255, 255, 255),
        sf::Color(203, 213, 225),
        sf::Color(2, 132, 199),
        sf::Color(15, 23, 42),
        sf::Color(148, 163, 184)
    );

    loginInputPass = std::unique_ptr<PasswordField>(new PasswordField(
        sf::Vector2f(rightX, cardY + 300.f),
        sf::Vector2f(rightW, 54.f),
        "••••••••••••",
        16
    ));
    loginInputPass->setTextInset(48.f);
    loginInputPass->setRightInset(48.f);
    loginInputPass->setCornerRadius(10.f);
    loginInputPass->setColors(
        sf::Color(255, 255, 255),
        sf::Color(203, 213, 225),
        sf::Color(2, 132, 199),
        sf::Color(15, 23, 42),
        sf::Color(148, 163, 184)
    );

    chkRememberMe = std::unique_ptr<UICheckboxItem>(new UICheckboxItem(
        "Ghi nhớ đăng nhập",
        sf::Vector2f(rightX, cardY + 374.f),
        true,
        15
    ));
    chkRememberMe->setColors(
        sf::Color(255, 255, 255),
        sf::Color(203, 213, 225),
        sf::Color(2, 132, 199),
        sf::Color(71, 85, 105)
    );

    btnLogin = std::unique_ptr<RoundRectButton>(new RoundRectButton(
        "ĐĂNG NHẬP HỆ THỐNG  →",
        sf::Vector2f(rightX, cardY + 416.f),
        sf::Vector2f(rightW, 54.f),
        ButtonStyle::Primary,
        10.f
    ));
    btnLogin->setCustomColors(
        sf::Color(2, 132, 199),
        sf::Color(3, 105, 161),
        sf::Color(7, 89, 133),
        sf::Color(203, 213, 225)
    );
    btnLogin->setFont(Theme::FontPrimary);
    btnLogin->setFontSize(17);

    // Handle button actions
    btnLogin->setOnClick([this]() {
        std::string u = loginInputUser->getText();
        std::string p = loginInputPass->getText();
        authenticateUser(u, p);
    });

    loginInputUser->setOnEnter([this](const std::string&) {
        if (loginInputPass) {
            loginInputUser->setFocused(false);
            loginInputPass->setFocused(true);
        }
    });

    loginInputPass->setOnEnter([this](const std::string& p) {
        std::string u = loginInputUser ? loginInputUser->getText() : "";
        authenticateUser(u, p);
    });
}

bool CinemaUIApp::authenticateUser(const std::string& inputUser, const std::string& inputPass) {
    if (inputUser.empty() || inputPass.empty()) {
        loginErrorMsg = "Vui lòng nhập đầy đủ tài khoản và mật khẩu!";
        return false;
    }

    // Read real users from SQLite DATA/cinema.db (users table)
    sqlite3* database = nullptr;
    if (sqlite3_open("DATA/cinema.db", &database) != SQLITE_OK || database == nullptr) {
        if (database != nullptr) sqlite3_close(database);
        loginErrorMsg = "Không thể mở cơ sở dữ liệu SQLite DATA/cinema.db!";
        return false;
    }

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "SELECT maNguoiDung,hoTen,email,taiKhoan,matKhau,vaiTro,trangThaiLamViec,trangThaiTaiKhoan "
        "FROM users WHERE (taiKhoan=? OR maNguoiDung=? OR email=?) AND matKhau=? ORDER BY rowid LIMIT 1";

    bool found = false;
    std::string statusErr = "";

    if (sqlite3_prepare_v2(database, sql, -1, &statement, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(statement, 1, inputUser.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(statement, 2, inputUser.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(statement, 3, inputUser.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(statement, 4, inputPass.c_str(), -1, SQLITE_TRANSIENT);

        if (sqlite3_step(statement) == SQLITE_ROW) {
            auto getCol = [&](int idx) -> std::string {
                const unsigned char* txt = sqlite3_column_text(statement, idx);
                return txt ? reinterpret_cast<const char*>(txt) : "";
            };
            std::string uid = getCol(0);
            std::string uname = getCol(1);
            std::string role = getCol(5);
            std::string workStatus = getCol(6);
            std::string accStatus = getCol(7);

            if (role == "Staff" && accStatus == "BiKhoa") {
                statusErr = "Tài khoản nhân viên đã bị khóa!";
            } else if (role == "Staff" && workStatus == "DaNghiViec") {
                statusErr = "Nhân viên đã nghỉ việc, không thể đăng nhập!";
            } else {
                currentUserId = uid;
                currentUserName = uname;
                currentUserRole = role;
                found = true;
            }
        }
        sqlite3_finalize(statement);
    }
    sqlite3_close(database);

    if (found) {
        isLoggedIn = true;
        loginErrorMsg = "";
        loadDataFromDatabase();

        if (currentUserRole == "Admin") {
            currentTab = ActiveTab::Reports;
        } else {
            currentTab = ActiveTab::SeatBooking;
        }
        setupNavigation();

        // Record real activity log into SQLite DATA/cinema.db
        ActivityLog::ghiNhan(currentUserId, currentUserRole, "DANG_NHAP", "Đăng nhập thành công qua giao diện UI");
        setupLogsView(); // refresh log table
        return true;
    } else {
        loginErrorMsg = statusErr.empty() ? "Tài khoản hoặc mật khẩu không chính xác!" : statusErr;
        return false;
    }
}


void CinemaUIApp::renderLoginView() {
    // 1. Draw modern clean blue-white canvas (Full Screen 1728x972)
    sf::RectangleShape bg(sf::Vector2f(1728.f, 972.f));
    bg.setFillColor(sf::Color(240, 244, 248)); // #f0f4f8 soft blue-white
    window.draw(bg);

    // Decorative soft blue ambient shapes in background
    sf::CircleShape glow1(380.f);
    glow1.setPosition(80.f, -120.f);
    glow1.setFillColor(sf::Color(224, 242, 254, 180)); // Soft sky blue #e0f2fe
    window.draw(glow1);

    sf::CircleShape glow2(420.f);
    glow2.setPosition(1100.f, 480.f);
    glow2.setFillColor(sf::Color(224, 242, 254, 160));
    window.draw(glow2);

    float cardW = 1020.f;
    float cardH = 560.f;
    float cardX = (1728.f - cardW) / 2.0f; // 354.f (Centered)
    float cardY = (972.f - cardH) / 2.0f;  // 206.f (Centered)
    float leftW = 420.f;
    float rightW = 520.f;
    float rightX = cardX + leftW + (cardW - leftW - rightW) / 2.0f; // 814.f (Centered in right panel)

    // 2. Drop shadow behind the card
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(cardX - 4.f, cardY + 6.f),
        sf::Vector2f(cardW + 8.f, cardH + 8.f),
        22.f,
        sf::Color(15, 23, 42, 25)
    );

    // 3. Main Crisp White Card Background
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(cardX, cardY),
        sf::Vector2f(cardW, cardH),
        16.f,
        sf::Color(255, 255, 255),
        1.f,
        sf::Color(226, 232, 240)
    );

    // 4. Left Panel Background: Soft Ice-Blue #f0f9ff
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(cardX, cardY),
        sf::Vector2f(leftW, cardH),
        16.f,
        sf::Color(240, 249, 255)
    );
    // Square off the right side of the left panel
    sf::RectangleShape leftFiller(sf::Vector2f(16.f, cardH));
    leftFiller.setPosition(cardX + leftW - 16.f, cardY);
    leftFiller.setFillColor(sf::Color(240, 249, 255));
    window.draw(leftFiller);

    // Left panel right separator line
    sf::RectangleShape leftSep(sf::Vector2f(1.f, cardH));
    leftSep.setPosition(cardX + leftW, cardY);
    leftSep.setFillColor(sf::Color(224, 242, 254));
    window.draw(leftSep);

    // 5. Top Blue Accent Strip
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(cardX, cardY),
        sf::Vector2f(cardW, 4.f),
        2.f,
        sf::Color(2, 132, 199)
    );

    // ==========================================
    // LEFT PANEL - BRANDING & SECURITY (CRISP)
    // ==========================================
    float leftCenterX = cardX + leftW / 2.0f;

    // 1. Logo Badge (Rounded Square with CINESTAR logo - unchanged)
    float badgeSize = 110.f;
    float badgeX = leftCenterX - badgeSize / 2.0f;
    float badgeY = cardY + 68.f;

    // Drop shadow behind logo badge
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(badgeX - 1.f, badgeY + 2.f),
        sf::Vector2f(badgeSize + 2.f, badgeSize + 2.f),
        22.f,
        sf::Color(2, 132, 199, 25)
    );

    RoundedRectRenderer::render(
        window,
        sf::Vector2f(badgeX, badgeY),
        sf::Vector2f(badgeSize, badgeSize),
        20.f,
        sf::Color(255, 255, 255),
        2.f,
        sf::Color(2, 132, 199)
    );

    if (cinemaLogoTexture.getSize().x > 0 && cinemaLogoTexture.getSize().y > 0) {
        sf::Vector2u texSz = cinemaLogoTexture.getSize();
        // Crop tightly around the central camera + curved CINESTAR logo
        int cropX = static_cast<int>(texSz.x * 0.24f);
        int cropY = static_cast<int>(texSz.y * 0.23f);
        int cropW = static_cast<int>(texSz.x * 0.52f);
        int cropH = static_cast<int>(texSz.y * 0.52f);
        cinemaLogoSprite.setTextureRect(sf::IntRect(cropX, cropY, cropW, cropH));
        float targetSize = 88.f;
        cinemaLogoSprite.setScale(
            targetSize / static_cast<float>(cropW),
            targetSize / static_cast<float>(cropH)
        );
        cinemaLogoSprite.setPosition(
            badgeX + (badgeSize - targetSize) / 2.0f,
            badgeY + (badgeSize - targetSize) / 2.0f
        );
        window.draw(cinemaLogoSprite);
    }

    // 2. Pill Badge: 🔑 CỔNG ĐIỀU HÀNH CỤM RẠP (perfectly centered icon + text)
    sf::Text pillText;
    pillText.setFont(Theme::FontPrimary);
    pillText.setCharacterSize(13);
    pillText.setStyle(sf::Text::Bold);
    pillText.setFillColor(sf::Color(3, 105, 161));
    pillText.setString(toSfString("CỔNG ĐIỀU HÀNH CỤM RẠP"));
    sf::FloatRect ptB = pillText.getLocalBounds();

    float iconW = 16.f;
    float iconGap = 9.f;
    float groupW = iconW + iconGap + ptB.width;
    float pillW = groupW + 38.f;
    float pillH = 34.f;
    float pillX = std::round(leftCenterX - pillW / 2.0f);
    float pillY = cardY + 206.f;

    RoundedRectRenderer::render(
        window,
        sf::Vector2f(pillX, pillY),
        sf::Vector2f(pillW, pillH),
        17.f,
        sf::Color(224, 242, 254)
    );

    float groupStartX = std::round(pillX + (pillW - groupW) / 2.0f);
    // Key icon inside pill
    sf::CircleShape keyPill(4.5f);
    keyPill.setPosition(groupStartX, pillY + (pillH - 9.f) / 2.0f);
    keyPill.setFillColor(sf::Color(2, 132, 199));
    window.draw(keyPill);

    sf::RectangleShape keyPillStem(sf::Vector2f(8.5f, 2.5f));
    keyPillStem.setPosition(groupStartX + 6.5f, pillY + (pillH - 2.5f) / 2.0f);
    keyPillStem.setFillColor(sf::Color(2, 132, 199));
    window.draw(keyPillStem);

    pillText.setPosition(
        std::round(groupStartX + iconW + iconGap - ptB.left),
        std::round(pillY + (pillH - ptB.height) / 2.0f - ptB.top)
    );
    window.draw(pillText);

    // 3. Brand Text: CINESTAR & Subtitle centered in the middle
    sf::Text brandText;
    brandText.setFont(Theme::FontCinema);
    brandText.setCharacterSize(54);
    brandText.setStyle(sf::Text::Bold);
    brandText.setFillColor(sf::Color(15, 23, 42));
    brandText.setString("CINESTAR");
    sf::FloatRect b1 = brandText.getLocalBounds();
    brandText.setPosition(
        std::round(leftCenterX - (b1.left + b1.width / 2.0f)),
        cardY + 262.f
    );
    window.draw(brandText);

    sf::Text brandSub;
    brandSub.setFont(Theme::FontPrimary);
    brandSub.setCharacterSize(14);
    brandSub.setStyle(sf::Text::Bold);
    brandSub.setFillColor(sf::Color(110, 124, 142));
    brandSub.setString(toSfString("HỆ THỐNG QUẢN LÝ RẠP CHIẾU PHIM"));
    sf::FloatRect bs = brandSub.getLocalBounds();
    brandSub.setPosition(
        std::round(leftCenterX - (bs.left + bs.width / 2.0f)),
        cardY + 340.f
    );
    window.draw(brandSub);

    // 4. Bottom Security Shield Badge Card (matching reference image)
    float secW = 364.f;
    float secH = 72.f;
    float secX = std::round(leftCenterX - secW / 2.0f);
    float secY = cardY + 448.f;

    RoundedRectRenderer::render(
        window,
        sf::Vector2f(secX, secY),
        sf::Vector2f(secW, secH),
        16.f,
        sf::Color(255, 255, 255),
        1.5f,
        sf::Color(210, 236, 252)
    );

    // Soft ice-blue rounded square icon container on the left
    float iconBoxSize = 40.f;
    float iconBoxX = secX + 16.f;
    float iconBoxY = std::round(secY + (secH - iconBoxSize) / 2.0f);
    sf::Color iconBoxBg(235, 248, 255);
    RoundedRectRenderer::render(
        window,
        sf::Vector2f(iconBoxX, iconBoxY),
        sf::Vector2f(iconBoxSize, iconBoxSize),
        10.f,
        iconBoxBg,
        1.f,
        sf::Color(205, 234, 254)
    );

    // 16x Supersampled Smooth Shield + Checkmark Icon Texture
    static sf::Texture shieldTex;
    static bool shieldTexReady = false;
    if (!shieldTexReady) {
        const int texS = 40;
        sf::Image shImg;
        shImg.create(texS, texS, sf::Color(235, 248, 255, 0));

        auto inShield = [](float x, float y, float sc) -> bool {
            x /= sc;
            y /= sc;
            float yTop = -10.2f + 0.042f * x * x;
            if (y < yTop || y > 11.2f) return false;
            float maxW = 9.4f;
            if (y > -6.5f) {
                float t = (y + 6.5f) / (11.2f + 6.5f);
                if (t > 1.f) return false;
                maxW = 9.4f * std::pow(std::max(0.f, 1.f - t * t), 0.58f);
            }
            return std::fabs(x) <= maxW;
        };

        auto distSeg = [](float px, float py, float ax, float ay, float bx, float by) -> float {
            float vx = bx - ax, vy = by - ay;
            float wx = px - ax, wy = py - ay;
            float c1 = vx * wx + vy * wy;
            if (c1 <= 0.f) return std::sqrt(wx * wx + wy * wy);
            float c2 = vx * vx + vy * vy;
            if (c2 <= c1) return std::sqrt((px - bx) * (px - bx) + (py - by) * (py - by));
            float b = c1 / c2;
            float pbx = ax + b * vx, pby = ay + b * vy;
            return std::sqrt((px - pbx) * (px - pbx) + (py - pby) * (py - pby));
        };

        for (int py = 0; py < texS; ++py) {
            for (int px = 0; px < texS; ++px) {
                int hits = 0;
                for (int sy = 0; sy < 4; ++sy) {
                    for (int sx = 0; sx < 4; ++sx) {
                        float fx = (px + (sx + 0.5f) * 0.25f) - texS * 0.5f;
                        float fy = (py + (sy + 0.5f) * 0.25f) - texS * 0.5f;
                        bool stroke = inShield(fx, fy, 1.0f) && !inShield(fx, fy, 0.78f);
                        float d1 = distSeg(fx, fy, -3.6f, 0.2f, -1.0f, 2.8f);
                        float d2 = distSeg(fx, fy, -1.0f, 2.8f,  4.0f, -2.4f);
                        bool check = (d1 <= 1.05f || d2 <= 1.05f);
                        if (stroke || check) hits++;
                    }
                }
                if (hits > 0) {
                    sf::Uint8 alpha = static_cast<sf::Uint8>((hits * 255) / 16);
                    shImg.setPixel(px, py, sf::Color(2, 132, 199, alpha));
                }
            }
        }
        shieldTex.loadFromImage(shImg);
        shieldTex.setSmooth(true);
        shieldTexReady = true;
    }

    sf::Sprite shieldSpr(shieldTex);
    shieldSpr.setPosition(iconBoxX, iconBoxY);
    window.draw(shieldSpr);

    float secTextX = iconBoxX + iconBoxSize + 14.f;
    float maxSecTextW = secW - (secTextX - secX) - 16.f;

    sf::Text secText1;
    secText1.setFont(Theme::FontPrimary);
    secText1.setCharacterSize(13);
    secText1.setFillColor(sf::Color(100, 116, 139));
    secText1.setString(toSfString("Hệ thống bảo mật nội bộ dành cho cán bộ &"));
    sf::FloatRect st1B = secText1.getLocalBounds();
    if (st1B.width > maxSecTextW && st1B.width > 0.f) {
        secText1.setScale(maxSecTextW / st1B.width, 1.f);
    }
    secText1.setPosition(std::round(secTextX), std::round(secY + 17.f));
    window.draw(secText1);

    sf::Text secText2;
    secText2.setFont(Theme::FontPrimary);
    secText2.setCharacterSize(13);
    secText2.setFillColor(sf::Color(100, 116, 139));
    secText2.setString(toSfString("nhân viên quản trị Cinestar"));
    secText2.setScale(secText1.getScale());
    secText2.setPosition(std::round(secTextX), std::round(secY + 38.f));
    window.draw(secText2);

    // ==========================================
    // RIGHT PANEL - FORM & INPUTS (CRISP WHITE)
    // ==========================================

    // 1. Heading
    sf::Text rTitle;
    rTitle.setFont(Theme::FontPrimary);
    rTitle.setCharacterSize(30);
    rTitle.setStyle(sf::Text::Bold);
    rTitle.setFillColor(sf::Color(15, 23, 42));
    rTitle.setString(toSfString("Đăng nhập hệ thống"));
    rTitle.setPosition(rightX, cardY + 70.f);
    window.draw(rTitle);

    sf::Text rSub;
    rSub.setFont(Theme::FontPrimary);
    rSub.setCharacterSize(15);
    rSub.setFillColor(sf::Color(100, 116, 139));
    rSub.setString(toSfString("Vui lòng xác thực tài khoản được cấp quyền quản trị cụm rạp"));
    rSub.setPosition(rightX, cardY + 114.f);
    window.draw(rSub);

    // 2. Field 1: TÀI KHOẢN NHÂN SỰ
    sf::RectangleShape idIconBg(sf::Vector2f(15.f, 13.f));
    idIconBg.setPosition(rightX, cardY + 170.f);
    idIconBg.setFillColor(sf::Color(2, 132, 199));
    window.draw(idIconBg);

    sf::Text idLabel;
    idLabel.setFont(Theme::FontPrimary);
    idLabel.setCharacterSize(14);
    idLabel.setStyle(sf::Text::Bold);
    idLabel.setFillColor(sf::Color(71, 85, 105));
    idLabel.setString(toSfString("TÀI KHOẢN NHÂN SỰ"));
    idLabel.setPosition(rightX + 22.f, cardY + 166.f);
    window.draw(idLabel);

    if (loginInputUser) {
        loginInputUser->draw(window);
        // Sharp inside user avatar icon 👤 (centered vertically inside 54px box)
        float uIconX = rightX + 18.f;
        float uIconY = cardY + 196.f + 19.f;

        sf::CircleShape uRing(9.f);
        uRing.setOrigin(9.f, 9.f);
        uRing.setPosition(uIconX + 8.f, uIconY + 8.f);
        uRing.setFillColor(sf::Color::Transparent);
        uRing.setOutlineThickness(1.5f);
        uRing.setOutlineColor(sf::Color(148, 163, 184));
        window.draw(uRing);

        sf::CircleShape uHead(2.8f);
        uHead.setOrigin(2.8f, 2.8f);
        uHead.setPosition(uIconX + 8.f, uIconY + 5.f);
        uHead.setFillColor(sf::Color(148, 163, 184));
        window.draw(uHead);

        sf::CircleShape uBody(4.5f);
        uBody.setOrigin(4.5f, 4.5f);
        uBody.setScale(1.f, 0.5f);
        uBody.setPosition(uIconX + 8.f, uIconY + 12.f);
        uBody.setFillColor(sf::Color(148, 163, 184));
        window.draw(uBody);
    }

    // 3. Field 2: MẬT KHẨU TRUY CẬP
    sf::RectangleShape lockBody(sf::Vector2f(13.f, 10.f));
    lockBody.setPosition(rightX, cardY + 274.f);
    lockBody.setFillColor(sf::Color(2, 132, 199));
    window.draw(lockBody);

    sf::CircleShape lockShackle(4.f);
    lockShackle.setPosition(rightX + 2.5f, cardY + 269.f);
    lockShackle.setFillColor(sf::Color::Transparent);
    lockShackle.setOutlineThickness(1.5f);
    lockShackle.setOutlineColor(sf::Color(2, 132, 199));
    window.draw(lockShackle);

    sf::Text passLabel;
    passLabel.setFont(Theme::FontPrimary);
    passLabel.setCharacterSize(14);
    passLabel.setStyle(sf::Text::Bold);
    passLabel.setFillColor(sf::Color(71, 85, 105));
    passLabel.setString(toSfString("MẬT KHẨU TRUY CẬP"));
    passLabel.setPosition(rightX + 22.f, cardY + 270.f);
    window.draw(passLabel);

    if (loginInputPass) {
        loginInputPass->draw(window);
        // Sharp inside key icon 🔑 (centered vertically inside 54px box)
        float kIconX = rightX + 18.f;
        float kIconY = cardY + 300.f + 21.f;

        sf::CircleShape kHead(4.5f);
        kHead.setOrigin(4.5f, 4.5f);
        kHead.setPosition(kIconX + 6.f, kIconY + 6.f);
        kHead.setFillColor(sf::Color::Transparent);
        kHead.setOutlineThickness(1.5f);
        kHead.setOutlineColor(sf::Color(148, 163, 184));
        window.draw(kHead);

        sf::RectangleShape kStem(sf::Vector2f(9.f, 2.f));
        kStem.setPosition(kIconX + 10.5f, kIconY + 5.f);
        kStem.setFillColor(sf::Color(148, 163, 184));
        window.draw(kStem);

        sf::RectangleShape kTooth(sf::Vector2f(2.f, 4.f));
        kTooth.setPosition(kIconX + 17.f, kIconY + 6.f);
        kTooth.setFillColor(sf::Color(148, 163, 184));
        window.draw(kTooth);
    }

    // 4. Remember Me Checkbox (without "Quên mật khẩu?")
    if (chkRememberMe) {
        chkRememberMe->draw(window);
    }

    // 5. Login Button: ĐĂNG NHẬP HỆ THỐNG ➔
    if (btnLogin) {
        btnLogin->draw(window);
    }

    // 6. Error message (if any)
    if (!loginErrorMsg.empty()) {
        sf::Text errText;
        errText.setFont(Theme::FontPrimary);
        errText.setCharacterSize(14);
        errText.setStyle(sf::Text::Bold);
        errText.setFillColor(sf::Color(220, 38, 38));
        errText.setString(toSfString(loginErrorMsg));
        errText.setPosition(rightX, cardY + 482.f);
        window.draw(errText);
    }
}

void CinemaUIApp::drawSidebarMenuIcon(sf::RenderTarget& window, int index, sf::Vector2f pos, sf::Color color) {
    switch (index) {
        case 0: { // 1. Bán vé (POS) - Ticket icon
            sf::RectangleShape ticket(sf::Vector2f(16.f, 11.f));
            ticket.setPosition(pos.x, pos.y + 2.f);
            ticket.setFillColor(sf::Color::Transparent);
            ticket.setOutlineThickness(1.3f);
            ticket.setOutlineColor(color);
            window.draw(ticket);

            sf::RectangleShape dash(sf::Vector2f(4.f, 1.5f));
            dash.setPosition(pos.x + 6.f, pos.y + 7.f);
            dash.setFillColor(color);
            window.draw(dash);
            break;
        }
        case 1: { // 2. Khách hàng (CRM) - User outline icon
            sf::CircleShape head(3.5f);
            head.setPosition(pos.x + 4.5f, pos.y + 0.5f);
            head.setFillColor(sf::Color::Transparent);
            head.setOutlineThickness(1.3f);
            head.setOutlineColor(color);
            window.draw(head);

            sf::CircleShape body(5.5f);
            body.setScale(1.f, 0.45f);
            body.setPosition(pos.x + 2.5f, pos.y + 8.5f);
            body.setFillColor(sf::Color::Transparent);
            body.setOutlineThickness(1.3f);
            body.setOutlineColor(color);
            window.draw(body);
            break;
        }
        case 2: { // 3. Quản lý Doanh thu - Wallet / Card
            sf::RectangleShape wallet(sf::Vector2f(16.f, 11.f));
            wallet.setPosition(pos.x, pos.y + 2.f);
            wallet.setFillColor(sf::Color::Transparent);
            wallet.setOutlineThickness(1.3f);
            wallet.setOutlineColor(color);
            window.draw(wallet);

            sf::CircleShape snap(1.5f);
            snap.setPosition(pos.x + 11.f, pos.y + 6.5f);
            snap.setFillColor(color);
            window.draw(snap);
            break;
        }
        case 3: { // 4. Quản lý phòng - Monitor / Cinema Screen
            sf::RectangleShape screen(sf::Vector2f(16.f, 10.f));
            screen.setPosition(pos.x, pos.y + 1.5f);
            screen.setFillColor(sf::Color::Transparent);
            screen.setOutlineThickness(1.3f);
            screen.setOutlineColor(color);
            window.draw(screen);

            sf::RectangleShape stand(sf::Vector2f(6.f, 1.5f));
            stand.setPosition(pos.x + 5.f, pos.y + 13.5f);
            stand.setFillColor(color);
            window.draw(stand);
            break;
        }
        case 4: { // 5. Quản lý ghế - 2x2 grid
            for (int r = 0; r < 2; ++r) {
                for (int c = 0; c < 2; ++c) {
                    sf::RectangleShape cell(sf::Vector2f(5.f, 5.f));
                    cell.setPosition(pos.x + c * 7.5f + 1.f, pos.y + r * 7.5f + 1.f);
                    cell.setFillColor(sf::Color::Transparent);
                    cell.setOutlineThickness(1.2f);
                    cell.setOutlineColor(color);
                    window.draw(cell);
                }
            }
            break;
        }
        case 5: { // 6. Quản lý suất chiếu - Clock
            sf::CircleShape clock(6.5f);
            clock.setPosition(pos.x + 1.5f, pos.y + 1.5f);
            clock.setFillColor(sf::Color::Transparent);
            clock.setOutlineThickness(1.3f);
            clock.setOutlineColor(color);
            window.draw(clock);

            sf::VertexArray hands(sf::Lines, 4);
            hands[0].position = sf::Vector2f(pos.x + 8.f, pos.y + 8.f);
            hands[1].position = sf::Vector2f(pos.x + 8.f, pos.y + 4.5f);
            hands[2].position = sf::Vector2f(pos.x + 8.f, pos.y + 8.f);
            hands[3].position = sf::Vector2f(pos.x + 11.f, pos.y + 8.f);
            for (int h = 0; h < 4; ++h) hands[h].color = color;
            window.draw(hands);
            break;
        }
        case 6: { // 7. Phim & Thể loại - Film strip
            sf::RectangleShape film(sf::Vector2f(16.f, 12.f));
            film.setPosition(pos.x, pos.y + 2.f);
            film.setFillColor(sf::Color::Transparent);
            film.setOutlineThickness(1.3f);
            film.setOutlineColor(color);
            window.draw(film);

            sf::RectangleShape hole1(sf::Vector2f(3.f, 2.f));
            hole1.setPosition(pos.x + 3.f, pos.y + 7.f);
            hole1.setFillColor(color);
            window.draw(hole1);

            sf::RectangleShape hole2(sf::Vector2f(3.f, 2.f));
            hole2.setPosition(pos.x + 10.f, pos.y + 7.f);
            hole2.setFillColor(color);
            window.draw(hole2);
            break;
        }
        case 7: { // 8. Quản lý F&B - Cup / Popcorn
            sf::ConvexShape cup(4);
            cup.setPoint(0, sf::Vector2f(pos.x + 2.f, pos.y + 2.5f));
            cup.setPoint(1, sf::Vector2f(pos.x + 14.f, pos.y + 2.5f));
            cup.setPoint(2, sf::Vector2f(pos.x + 11.5f, pos.y + 14.f));
            cup.setPoint(3, sf::Vector2f(pos.x + 4.5f, pos.y + 14.f));
            cup.setFillColor(sf::Color::Transparent);
            cup.setOutlineThickness(1.3f);
            cup.setOutlineColor(color);
            window.draw(cup);

            sf::RectangleShape stripe(sf::Vector2f(1.5f, 9.f));
            stripe.setPosition(pos.x + 7.25f, pos.y + 4.f);
            stripe.setFillColor(color);
            window.draw(stripe);
            break;
        }
        case 8: { // 9. Quản lý Báo cáo ca - Clipboard
            sf::RectangleShape clip(sf::Vector2f(13.f, 14.f));
            clip.setPosition(pos.x + 1.5f, pos.y + 2.f);
            clip.setFillColor(sf::Color::Transparent);
            clip.setOutlineThickness(1.3f);
            clip.setOutlineColor(color);
            window.draw(clip);

            sf::RectangleShape topTab(sf::Vector2f(5.f, 2.5f));
            topTab.setPosition(pos.x + 5.5f, pos.y + 0.5f);
            topTab.setFillColor(color);
            window.draw(topTab);

            sf::RectangleShape line1(sf::Vector2f(7.f, 1.3f));
            line1.setPosition(pos.x + 4.5f, pos.y + 6.5f);
            line1.setFillColor(color);
            window.draw(line1);

            sf::RectangleShape line2(sf::Vector2f(7.f, 1.3f));
            line2.setPosition(pos.x + 4.5f, pos.y + 9.5f);
            line2.setFillColor(color);
            window.draw(line2);
            break;
        }
        case 9: { // 10. Khuyến mãi SK - Gift box
            sf::RectangleShape gift(sf::Vector2f(14.f, 11.f));
            gift.setPosition(pos.x + 1.f, pos.y + 3.5f);
            gift.setFillColor(sf::Color::Transparent);
            gift.setOutlineThickness(1.3f);
            gift.setOutlineColor(color);
            window.draw(gift);

            sf::RectangleShape ribH(sf::Vector2f(14.f, 1.5f));
            ribH.setPosition(pos.x + 1.f, pos.y + 8.f);
            ribH.setFillColor(color);
            window.draw(ribH);

            sf::RectangleShape ribV(sf::Vector2f(1.5f, 11.f));
            ribV.setPosition(pos.x + 7.25f, pos.y + 3.5f);
            ribV.setFillColor(color);
            window.draw(ribV);
            break;
        }
        case 10: { // 11. Quản lý nhân sự - Group
            sf::CircleShape h2(3.f);
            h2.setPosition(pos.x + 8.5f, pos.y + 1.f);
            h2.setFillColor(color);
            window.draw(h2);

            sf::CircleShape h1(3.5f);
            h1.setPosition(pos.x + 2.f, pos.y + 2.5f);
            h1.setFillColor(sf::Color::Transparent);
            h1.setOutlineThickness(1.3f);
            h1.setOutlineColor(color);
            window.draw(h1);

            sf::CircleShape b1(5.f);
            b1.setScale(1.f, 0.45f);
            b1.setPosition(pos.x + 0.5f, pos.y + 9.5f);
            b1.setFillColor(sf::Color::Transparent);
            b1.setOutlineThickness(1.3f);
            b1.setOutlineColor(color);
            window.draw(b1);
            break;
        }
        case 11: { // 12. Quản lý tài khoản - Gear
            sf::CircleShape gear(6.f);
            gear.setPosition(pos.x + 2.f, pos.y + 2.5f);
            gear.setFillColor(sf::Color::Transparent);
            gear.setOutlineThickness(1.8f);
            gear.setOutlineColor(color);
            window.draw(gear);

            sf::CircleShape center(1.5f);
            center.setPosition(pos.x + 6.5f, pos.y + 7.f);
            center.setFillColor(color);
            window.draw(center);
            break;
        }
        case 12: { // 13. Nhật ký hoạt động - History / Clock
            sf::CircleShape logClock(6.5f);
            logClock.setPosition(pos.x + 1.5f, pos.y + 2.f);
            logClock.setFillColor(sf::Color::Transparent);
            logClock.setOutlineThickness(1.3f);
            logClock.setOutlineColor(color);
            window.draw(logClock);

            sf::VertexArray lHands(sf::Lines, 4);
            lHands[0].position = sf::Vector2f(pos.x + 8.f, pos.y + 8.5f);
            lHands[1].position = sf::Vector2f(pos.x + 8.f, pos.y + 5.f);
            lHands[2].position = sf::Vector2f(pos.x + 8.f, pos.y + 8.5f);
            lHands[3].position = sf::Vector2f(pos.x + 11.f, pos.y + 8.5f);
            for (int h = 0; h < 4; ++h) lHands[h].color = color;
            window.draw(lHands);
            break;
        }
    }
}

} // namespace UI