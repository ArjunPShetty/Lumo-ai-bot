import 'package:flutter/material.dart';

class HistoryPage extends StatelessWidget {
  const HistoryPage({Key? key}) : super(key: key);

  @override
  Widget build(BuildContext context) {
    // Example history data
    final List<Map<String, dynamic>> chatHistory = [
      {
        "title": "Chat with LUMO - Sep 10, 2025",
        "messages": [
          "You: What is AI?",
          "LUMO: AI stands for Artificial Intelligence."
        ]
      },
      {
        "title": "Chat with LUMO - Sep 8, 2025",
        "messages": [
          "You: Tell me a joke",
          "LUMO: Why don’t robots get tired? Because they recharge!"
        ]
      },
      {
        "title": "Chat with LUMO - Sep 5, 2025",
        "messages": [
          "You: Who are you?",
          "LUMO: I am your assistant, LUMO."
        ]
      },
    ];

    return Scaffold(
      appBar: AppBar(
        backgroundColor: Colors.white,
        elevation: 2,
        leading: IconButton(
          icon: const Icon(Icons.arrow_back, color: Colors.deepPurple),
          onPressed: () => Navigator.pop(context),
        ),
        title: Row(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            Image.asset('assets/logo.png', height: 36),
            const SizedBox(width: 10),
            const Text(
              'History',
              style: TextStyle(
                fontWeight: FontWeight.bold,
                fontSize: 22,
                color: Colors.black,
                letterSpacing: 1.1,
              ),
            ),
          ],
        ),
        centerTitle: true,
      ),
      body: ListView.builder(
        padding: const EdgeInsets.all(14),
        itemCount: chatHistory.length,
        itemBuilder: (context, index) {
          final chat = chatHistory[index];
          return Card(
            margin: const EdgeInsets.symmetric(vertical: 8),
            shape: RoundedRectangleBorder(
              borderRadius: BorderRadius.circular(16),
            ),
            elevation: 3,
            child: ListTile(
              contentPadding:
              const EdgeInsets.symmetric(horizontal: 16, vertical: 12),
              leading: const CircleAvatar(
                backgroundColor: Colors.deepPurple,
                child: Icon(Icons.chat, color: Colors.white),
              ),
              title: Text(
                chat["title"],
                style: const TextStyle(
                  fontWeight: FontWeight.w600,
                  fontSize: 16,
                ),
              ),
              subtitle: Text(
                (chat["messages"] as List).join("\n"),
                maxLines: 2,
                overflow: TextOverflow.ellipsis,
                style: const TextStyle(color: Colors.black87),
              ),
              onTap: () {
                Navigator.push(
                  context,
                  MaterialPageRoute(
                    builder: (_) => ChatDetailPage(chat: chat),
                  ),
                );
              },
            ),
          );
        },
      ),
    );
  }
}

class ChatDetailPage extends StatelessWidget {
  final Map<String, dynamic> chat;

  const ChatDetailPage({Key? key, required this.chat}) : super(key: key);

  @override
  Widget build(BuildContext context) {
    final messages = chat["messages"] as List;

    return Scaffold(
      appBar: AppBar(
        backgroundColor: Colors.white,
        elevation: 2,
        leading: IconButton(
          icon: const Icon(Icons.arrow_back, color: Colors.deepPurple),
          onPressed: () => Navigator.pop(context),
        ),
        title: Text(
          chat["title"],
          style: const TextStyle(
            fontWeight: FontWeight.bold,
            fontSize: 20,
            color: Colors.black,
          ),
        ),
        centerTitle: true,
      ),
      body: ListView.builder(
        padding: const EdgeInsets.all(16),
        itemCount: messages.length,
        itemBuilder: (context, index) {
          final msg = messages[index];
          final isUser = msg.startsWith("You:");

          return Container(
            margin: const EdgeInsets.symmetric(vertical: 6),
            alignment: isUser ? Alignment.centerRight : Alignment.centerLeft,
            child: Container(
              padding:
              const EdgeInsets.symmetric(vertical: 10, horizontal: 14),
              decoration: BoxDecoration(
                color: isUser ? Colors.deepPurple : Colors.white,
                borderRadius: BorderRadius.only(
                  topLeft: const Radius.circular(16),
                  topRight: const Radius.circular(16),
                  bottomLeft: Radius.circular(isUser ? 16 : 0),
                  bottomRight: Radius.circular(isUser ? 0 : 16),
                ),
                boxShadow: [
                  BoxShadow(
                    color: Colors.grey.withOpacity(0.15),
                    blurRadius: 5,
                    offset: const Offset(0, 3),
                  ),
                ],
              ),
              child: Text(
                msg,
                style: TextStyle(
                  color: isUser ? Colors.white : Colors.black87,
                  fontSize: 15,
                ),
              ),
            ),
          );
        },
      ),
    );
  }
}
