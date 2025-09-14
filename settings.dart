import 'package:flutter/material.dart';
import 'package:shared_preferences/shared_preferences.dart';

class SettingsPage extends StatefulWidget {
  @override
  _SettingsPageState createState() => _SettingsPageState();
}

class _SettingsPageState extends State<SettingsPage> {
  bool _darkMode = false;
  String _themeMode = "System"; // System, Light, Dark
  bool _notifications = true;
  bool _chatNotifications = true;
  bool _updateNotifications = true;
  bool _reminderNotifications = false;
  String _language = 'English';
  bool _biometricLock = false;

  String _username = "User Name";
  String _email = "user@email.com";

  @override
  void initState() {
    super.initState();
    _loadSettings();
  }

  Future<void> _loadSettings() async {
    SharedPreferences prefs = await SharedPreferences.getInstance();
    setState(() {
      _darkMode = prefs.getBool('darkMode') ?? false;
      _themeMode = prefs.getString('themeMode') ?? "System";
      _notifications = prefs.getBool('notifications') ?? true;
      _chatNotifications = prefs.getBool('chatNotifications') ?? true;
      _updateNotifications = prefs.getBool('updateNotifications') ?? true;
      _reminderNotifications = prefs.getBool('reminderNotifications') ?? false;
      _language = prefs.getString('language') ?? 'English';
      _biometricLock = prefs.getBool('biometricLock') ?? false;
      _username = prefs.getString('username') ?? "User Name";
      _email = prefs.getString('email') ?? "user@email.com";
    });
  }

  Future<void> _saveSettings() async {
    SharedPreferences prefs = await SharedPreferences.getInstance();
    await prefs.setBool('darkMode', _darkMode);
    await prefs.setString('themeMode', _themeMode);
    await prefs.setBool('notifications', _notifications);
    await prefs.setBool('chatNotifications', _chatNotifications);
    await prefs.setBool('updateNotifications', _updateNotifications);
    await prefs.setBool('reminderNotifications', _reminderNotifications);
    await prefs.setString('language', _language);
    await prefs.setBool('biometricLock', _biometricLock);
    await prefs.setString('username', _username);
    await prefs.setString('email', _email);
  }

  void _clearChatHistory() async {
    showDialog(
      context: context,
      builder: (_) => AlertDialog(
        title: Text('Clear Chat History'),
        content: Text('Are you sure you want to delete all chat history?'),
        actions: [
          TextButton(onPressed: () => Navigator.pop(context), child: Text('Cancel')),
          TextButton(
            onPressed: () async {
              SharedPreferences prefs = await SharedPreferences.getInstance();
              await prefs.remove('chatHistory');
              Navigator.pop(context);
              ScaffoldMessenger.of(context).showSnackBar(
                SnackBar(content: Text('Chat history cleared')),
              );
            },
            child: Text('Clear', style: TextStyle(color: Colors.red)),
          ),
        ],
      ),
    );
  }

  void _editProfile() {
    TextEditingController nameCtrl = TextEditingController(text: _username);
    TextEditingController emailCtrl = TextEditingController(text: _email);

    showDialog(
      context: context,
      builder: (_) => AlertDialog(
        title: Text("Edit Profile"),
        content: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            TextField(controller: nameCtrl, decoration: InputDecoration(labelText: "Name")),
            TextField(controller: emailCtrl, decoration: InputDecoration(labelText: "Email")),
          ],
        ),
        actions: [
          TextButton(onPressed: () => Navigator.pop(context), child: Text("Cancel")),
          ElevatedButton(
            onPressed: () {
              setState(() {
                _username = nameCtrl.text;
                _email = emailCtrl.text;
                _saveSettings();
              });
              Navigator.pop(context);
            },
            child: Text("Save"),
          )
        ],
      ),
    );
  }

  void _exportChatHistory() {
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(content: Text('Chat history exported (mock action)')),
    );
  }

  void _importChatHistory() {
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(content: Text('Chat history imported (mock action)')),
    );
  }

  void _showLicenses() {
    showLicensePage(context: context, applicationName: "LUMA", applicationVersion: "1.0.0");
  }

  void _sendFeedback() {
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(content: Text('Redirect to feedback form (mock action)')),
    );
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text('Settings'),
        backgroundColor: Colors.deepPurple,
      ),
      body: ListView(
        padding: EdgeInsets.all(16),
        children: [
          // Profile Section
          GestureDetector(
            onTap: _editProfile,
            child: Container(
              padding: EdgeInsets.all(16),
              decoration: BoxDecoration(
                color: Colors.indigo.shade50,
                borderRadius: BorderRadius.circular(16),
              ),
              child: Row(
                children: [
                  CircleAvatar(
                    radius: 30,
                    backgroundColor: Colors.deepPurple,
                    child: Icon(Icons.person, color: Colors.white, size: 30),
                  ),
                  SizedBox(width: 16),
                  Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Text(_username,
                          style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold)),
                      SizedBox(height: 4),
                      Text(_email, style: TextStyle(fontSize: 14)),
                    ],
                  )
                ],
              ),
            ),
          ),
          SizedBox(height: 20),

          // Theme Mode
          ListTile(
            leading: Icon(Icons.color_lens, color: Colors.deepPurple),
            title: Text('Theme Mode'),
            trailing: DropdownButton<String>(
              value: _themeMode,
              items: ["System", "Light", "Dark"].map((val) {
                return DropdownMenuItem(value: val, child: Text(val));
              }).toList(),
              onChanged: (val) {
                setState(() {
                  _themeMode = val!;
                  _darkMode = _themeMode == "Dark";
                  _saveSettings();
                });
              },
            ),
          ),
          Divider(),

          // Notifications master switch
          SwitchListTile(
            secondary: Icon(Icons.notifications, color: Colors.deepPurple),
            title: Text("Enable Notifications"),
            value: _notifications,
            onChanged: (val) {
              setState(() {
                _notifications = val;
                _saveSettings();
              });
            },
          ),
          if (_notifications) ...[
            SwitchListTile(
              secondary: Icon(Icons.chat, color: Colors.deepPurple),
              title: Text("Chat Notifications"),
              value: _chatNotifications,
              onChanged: (val) {
                setState(() {
                  _chatNotifications = val;
                  _saveSettings();
                });
              },
            ),
            SwitchListTile(
              secondary: Icon(Icons.system_update, color: Colors.deepPurple),
              title: Text("Update Notifications"),
              value: _updateNotifications,
              onChanged: (val) {
                setState(() {
                  _updateNotifications = val;
                  _saveSettings();
                });
              },
            ),
            SwitchListTile(
              secondary: Icon(Icons.alarm, color: Colors.deepPurple),
              title: Text("Reminders"),
              value: _reminderNotifications,
              onChanged: (val) {
                setState(() {
                  _reminderNotifications = val;
                  _saveSettings();
                });
              },
            ),
          ],
          Divider(),

          // Language
          ListTile(
            leading: Icon(Icons.language, color: Colors.deepPurple),
            title: Text('Language'),
            trailing: DropdownButton<String>(
              value: _language,
              items: <String>['English', 'Spanish', 'French', 'Hindi', 'German']
                  .map((String val) {
                return DropdownMenuItem<String>(
                  value: val,
                  child: Text(val),
                );
              }).toList(),
              onChanged: (val) {
                setState(() {
                  _language = val!;
                  _saveSettings();
                });
              },
            ),
          ),
          Divider(),

          // Security
          SwitchListTile(
            secondary: Icon(Icons.lock, color: Colors.deepPurple),
            title: Text("Enable Biometric Lock"),
            value: _biometricLock,
            onChanged: (val) {
              setState(() {
                _biometricLock = val;
                _saveSettings();
              });
            },
          ),
          Divider(),

          // Data Management
          ListTile(
            leading: Icon(Icons.delete_forever, color: Colors.red),
            title: Text('Clear Chat History', style: TextStyle(color: Colors.red)),
            onTap: _clearChatHistory,
          ),
          ListTile(
            leading: Icon(Icons.upload, color: Colors.deepPurple),
            title: Text('Export Chat History'),
            onTap: _exportChatHistory,
          ),
          ListTile(
            leading: Icon(Icons.download, color: Colors.deepPurple),
            title: Text('Import Chat History'),
            onTap: _importChatHistory,
          ),
          Divider(),

          // About Section
          ListTile(
            leading: Icon(Icons.info_outline, color: Colors.deepPurple),
            title: Text('App Version'),
            trailing: Text('1.0.0'),
          ),
          ListTile(
            leading: Icon(Icons.article, color: Colors.deepPurple),
            title: Text('Open Source Licenses'),
            onTap: _showLicenses,
          ),
          ListTile(
            leading: Icon(Icons.feedback, color: Colors.deepPurple),
            title: Text('Send Feedback'),
            onTap: _sendFeedback,
          ),
        ],
      ),
    );
  }
}
