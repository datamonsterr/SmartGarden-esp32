import 'package:flutter/material.dart';
import 'package:thingsboard_app/utils/transition/page_transitions.dart';

// --- PHẦN 1: CẤU HÌNH MÀU SẮC (Đã đổi sang Xanh Lá) ---
const int _tbPrimaryColorValue = 0xFF4CAF50; // Màu xanh lá chủ đạo (Green 500)
const Color _tbPrimaryColor = Color(_tbPrimaryColorValue);
const Color _tbSecondaryColor = Color(0xFF2E7D32); // Xanh đậm hơn (Green 800)
const Color _tbDarkPrimaryColor = Color(0xFF81C784); // Xanh nhạt (Dùng cho dark mode)

Color get appPrimaryColor => _tbPrimaryColor;

// Màu chữ giữ nguyên
const int _tbTextColorValue = 0xFF282828;
const Color _tbTextColor = Color(_tbTextColorValue);

Typography tbTypography = Typography.material2018();

// Tạo bảng màu (Swatch) Xanh Lá thay vì Xanh Dương cũ
const tbMatGreen = MaterialColor(_tbPrimaryColorValue, <int, Color>{
  50: Color(0xFFE8F5E9),
  100: Color(0xFFC8E6C9),
  200: Color(0xFFA5D6A7),
  300: Color(0xFF81C784),
  400: Color(0xFF66BB6A),
  500: _tbPrimaryColor,
  600: _tbSecondaryColor,
  700: Color(0xFF388E3C),
  800: Color(0xFF2E7D32),
  900: Color(0xFF1B5E20),
});

// Bảng màu cho Dark Mode
const tbDarkMatGreen = MaterialColor(_tbPrimaryColorValue, <int, Color>{
  50: Color(0xFFE8F5E9),
  100: Color(0xFFC8E6C9),
  200: Color(0xFFA5D6A7),
  300: Color(0xFF81C784),
  400: Color(0xFF66BB6A),
  500: _tbDarkPrimaryColor,
  600: _tbSecondaryColor,
  700: Color(0xFF388E3C),
  800: _tbPrimaryColor,
  900: Color(0xFF1B5E20),
});

final ThemeData theme = ThemeData(primarySwatch: tbMatGreen);

// --- PHẦN 2: CẤU HÌNH THEME ---
ThemeData tbTheme = ThemeData(
  useMaterial3: false,
  primarySwatch: tbMatGreen, // Dùng bảng màu xanh lá
  colorScheme: theme.colorScheme.copyWith(
    primary: tbMatGreen,
    secondary: Colors.orange, // Đổi accent sang màu cam (như quả chín/nắng) cho nổi
  ),
  scaffoldBackgroundColor: const Color(0xFFFAFAFA), // Màu nền hơi xám nhẹ
  textTheme: tbTypography.black,
  primaryTextTheme: tbTypography.black,
  typography: tbTypography,
  
  // Cấu hình thanh tiêu đề (AppBar)
  appBarTheme: const AppBarTheme(
    backgroundColor: Colors.white,
    foregroundColor: _tbTextColor, // Chữ màu đen
    iconTheme: IconThemeData(color: _tbTextColor), // Icon màu đen
  ),
  
  // Cấu hình thanh menu dưới đáy (BottomBar)
  bottomNavigationBarTheme: BottomNavigationBarThemeData(
    backgroundColor: Colors.white,
    selectedItemColor: _tbPrimaryColor, // Mục được chọn sẽ màu Xanh Lá
    unselectedItemColor: Colors.grey,   // Mục chưa chọn màu xám
    showSelectedLabels: true,
    showUnselectedLabels: true,
  ),
  
  // Hiệu ứng chuyển trang
  pageTransitionsTheme: const PageTransitionsTheme(
    builders: {
      TargetPlatform.iOS: FadeOpenPageTransitionsBuilder(),
      TargetPlatform.android: FadeOpenPageTransitionsBuilder(),
    },
  ),
);

// --- PHẦN 3: DARK MODE (Giữ nguyên cấu trúc, chỉ thay màu) ---
final ThemeData darkTheme = ThemeData(
  primarySwatch: tbDarkMatGreen,
  brightness: Brightness.dark,
);

ThemeData tbDarkTheme = ThemeData(
  primarySwatch: tbDarkMatGreen,
  colorScheme: darkTheme.colorScheme.copyWith(secondary: Colors.deepOrange),
  brightness: Brightness.dark,
);