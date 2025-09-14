import 'package:flutter/material.dart';
import 'settings.dart';
import 'about.dart';
import 'newpage.dart';
import 'historypage.dart';

class AppDrawer extends StatelessWidget {
  final VoidCallback onToggleTheme;
  final bool isDarkMode;

  const AppDrawer({
    Key? key,
    required this.onToggleTheme,
    required this.isDarkMode,
  }) : super(key: key);

  @override
  Widget build(BuildContext context) {
    return Drawer(
      child: ListView(
        padding: EdgeInsets.zero,
        children: [
          DrawerHeader(
            decoration: const BoxDecoration(
              color: Colors.deepPurple,
            ),
            child: Column(
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                Container(
                  height: 60,
                  width: 60,
                  decoration: const BoxDecoration(
                    shape: BoxShape.circle,
                    color: Colors.white,
                  ),
                  child: Padding(
                    padding: const EdgeInsets.all(8.0),
                    child: Image.asset(
                      'assets/logo.png',
                      fit: BoxFit.contain,
                    ),
                  ),
                ),
                const SizedBox(height: 10),
                const Text(
                  "LUMA",
                  style: TextStyle(
                    color: Colors.white,
                    fontSize: 20,
                    fontWeight: FontWeight.bold,
                    letterSpacing: 1.2,
                  ),
                ),
              ],
            ),
          ),
          ListTile(
            leading: const Icon(Icons.home, color: Colors.deepPurple),
            title: const Text("Home"),
            onTap: () {
              Navigator.pop(context);
            },
          ),
          ListTile(
            leading: const Icon(Icons.history, color: Colors.deepPurple),
            title: const Text("History"),
            onTap: () {
              Navigator.push(
                context,
                MaterialPageRoute(builder: (context) => const HistoryPage()),
              );
            },
          ),
          ListTile(
            leading: const Icon(Icons.create, color: Colors.deepPurple),
            title: const Text("New Page"),
            onTap: () {
              Navigator.push(
                context,
                MaterialPageRoute(
                  builder: (context) => NewPage(
                    onToggleTheme: onToggleTheme,
                    isDarkMode: isDarkMode,
                  ),
                ),
              );
            },
          ),
          ListTile(
            leading: const Icon(Icons.settings, color: Colors.deepPurple),
            title: const Text("Settings"),
            onTap: () {
              Navigator.push(
                context,
                MaterialPageRoute(builder: (context) => SettingsPage()),
              );
            },
          ),
          ListTile(
            leading: const Icon(Icons.info, color: Colors.deepPurple),
            title: const Text("About"),
            onTap: () {
              Navigator.push(
                context,
                MaterialPageRoute(builder: (context) => AboutPage()),
              );
            },
          ),
          const Divider(),
          SwitchListTile(
            secondary: const Icon(Icons.dark_mode, color: Colors.deepPurple),
            title: const Text("Dark Mode"),
            value: isDarkMode,
            onChanged: (_) => onToggleTheme(),
          ),
        ],
      ),
    );
  }
}
