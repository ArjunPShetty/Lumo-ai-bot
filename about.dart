import 'package:flutter/material.dart';

class AboutPage extends StatelessWidget {
  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        backgroundColor: Colors.indigo,
        title: Text('About Lumo AI Bot'),
        centerTitle: true,
      ),
      body: Padding(
        padding: EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Center(
              child: CircleAvatar(
                radius: 50,
                backgroundColor: Colors.indigo,
                child: Text(
                  'L',
                  style: TextStyle(
                      fontSize: 50, color: Colors.white, fontWeight: FontWeight.bold),
                ),
              ),
            ),
            SizedBox(height: 16),
            Center(
              child: Text(
                'Lumo AI Bot',
                style: TextStyle(fontSize: 26, fontWeight: FontWeight.bold),
              ),
            ),
            SizedBox(height: 8),
            Center(
              child: Text(
                'Your personal AI assistant',
                style: TextStyle(fontSize: 16, color: Colors.grey[600]),
              ),
            ),
            SizedBox(height: 24),
            Text(
              'About the App',
              style: TextStyle(fontSize: 20, fontWeight: FontWeight.bold),
            ),
            SizedBox(height: 8),
            Text(
              'Lumo AI Bot is an intelligent chat assistant designed to answer your questions, '
                  'help with tasks, and provide information quickly and accurately. Our mission '
                  'is to create a seamless and intuitive AI experience for all users.',
              style: TextStyle(fontSize: 16, color: Colors.grey[800]),
            ),
            SizedBox(height: 24),
            Text(
              'Developer',
              style: TextStyle(fontSize: 20, fontWeight: FontWeight.bold),
            ),
            SizedBox(height: 8),
            Text(
              'Developed by Your Name / Company Name.\n'
                  'Email: developer@example.com\n'
                  'Website: www.example.com',
              style: TextStyle(fontSize: 16, color: Colors.grey[800]),
            ),
            Spacer(),
            Center(
              child: Text(
                'Version 1.0.0',
                style: TextStyle(color: Colors.grey[600], fontSize: 14),
              ),
            ),
          ],
        ),
      ),
    );
  }
}
