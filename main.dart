import 'package:flutter/material.dart';
import 'app_drawer.dart';

void main() {
  runApp(LumaApp());
}

class LumaApp extends StatefulWidget {
  @override
  State<LumaApp> createState() => _LumaAppState();
}

class _LumaAppState extends State<LumaApp> {
  bool _isDarkMode = false;

  void _toggleTheme() {
    setState(() {
      _isDarkMode = !_isDarkMode;
    });
  }

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      debugShowCheckedModeBanner: false,
      title: 'LUMA',
      theme: _isDarkMode ? ThemeData.dark() : ThemeData.light(),
      home: HomePage(
        onToggleTheme: _toggleTheme,
        isDarkMode: _isDarkMode,
      ),
    );
  }
}

class HomePage extends StatefulWidget {
  final VoidCallback onToggleTheme;
  final bool isDarkMode;

  const HomePage({
    Key? key,
    required this.onToggleTheme,
    required this.isDarkMode,
  }) : super(key: key);

  @override
  _HomePageState createState() => _HomePageState();
}

class _HomePageState extends State<HomePage> {
  final TextEditingController _controller = TextEditingController();
  final List<ChatMessage> _messages = [];
  bool _hasChatted = false;

  void _sendMessage() {
    if (_controller.text.trim().isEmpty) return;
    setState(() {
      _hasChatted = true;
      _messages.add(ChatMessage(text: _controller.text, isUser: true));
      _messages.add(ChatMessage(
          text: "This is Luma response to '${_controller.text}'",
          isUser: false));
      _controller.clear();
    });
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      drawer: AppDrawer(
        onToggleTheme: widget.onToggleTheme,
        isDarkMode: widget.isDarkMode,
      ),
      appBar: _buildAppBar(),
      body: Column(
        children: [
          if (!_hasChatted)
            Expanded(
              child: Center(
                child: Container(
                  width: MediaQuery.of(context).size.width * 0.85,
                  padding: const EdgeInsets.symmetric(horizontal: 20, vertical: 6),
                  decoration: BoxDecoration(
                    color: Colors.white,
                    borderRadius: BorderRadius.circular(32),
                    boxShadow: [
                      BoxShadow(
                        color: Colors.black12,
                        blurRadius: 6,
                        offset: const Offset(0, 3),
                      )
                    ],
                  ),
                  child: TextField(
                    controller: _controller,
                    textAlign: TextAlign.center,
                    style: const TextStyle(fontSize: 16),
                    decoration: InputDecoration(
                      hintText: "Ask Luma something...",
                      border: InputBorder.none,
                      prefixIcon: const Icon(Icons.search, color: Colors.deepPurple),
                    ),
                    onSubmitted: (_) => _sendMessage(),
                  ),
                ),
              ),
            ),
          if (_hasChatted)
            Expanded(
              child: ListView.builder(
                padding: const EdgeInsets.all(14),
                itemCount: _messages.length,
                itemBuilder: (context, index) {
                  return _messages[index];
                },
              ),
            ),
          if (_hasChatted) _buildChatInput(),
        ],
      ),
    );
  }

  PreferredSizeWidget _buildAppBar() {
    return AppBar(
      backgroundColor: Colors.white,
      elevation: 2,
      iconTheme: const IconThemeData(color: Colors.deepPurple),
      title: Row(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          Image.asset('assets/logo.png', height: 36),
          const SizedBox(width: 10),
          const Text(
            'LUMA',
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
    );
  }

  Widget _buildChatInput() {
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 10),
      color: Colors.white,
      child: Row(
        children: [
          Expanded(
            child: TextField(
              controller: _controller,
              style: const TextStyle(fontSize: 16),
              decoration: InputDecoration(
                hintText: "Type your message...",
                filled: true,
                fillColor: Colors.grey[200],
                contentPadding:
                const EdgeInsets.symmetric(horizontal: 16, vertical: 12),
                border: OutlineInputBorder(
                  borderRadius: BorderRadius.circular(32),
                  borderSide: BorderSide.none,
                ),
              ),
              onSubmitted: (_) => _sendMessage(),
            ),
          ),
          const SizedBox(width: 10),
          FloatingActionButton(
            onPressed: _sendMessage,
            backgroundColor: Colors.deepPurple,
            child: const Icon(Icons.send, color: Colors.white),
            mini: true,
          )
        ],
      ),
    );
  }
}

class ChatMessage extends StatelessWidget {
  final String text;
  final bool isUser;

  const ChatMessage({required this.text, required this.isUser});

  @override
  Widget build(BuildContext context) {
    return Container(
      margin: const EdgeInsets.symmetric(vertical: 6),
      child: Row(
        mainAxisAlignment:
        isUser ? MainAxisAlignment.end : MainAxisAlignment.start,
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          if (!isUser)
            const CircleAvatar(
              backgroundColor: Colors.deepPurple,
              child: Text('L', style: TextStyle(color: Colors.white)),
            ),
          if (!isUser) const SizedBox(width: 10),
          Flexible(
            child: Container(
              padding: const EdgeInsets.symmetric(vertical: 12, horizontal: 16),
              decoration: BoxDecoration(
                color: isUser ? Colors.deepPurple : Colors.white,
                borderRadius: BorderRadius.only(
                  topLeft: const Radius.circular(20),
                  topRight: const Radius.circular(20),
                  bottomLeft: Radius.circular(isUser ? 20 : 0),
                  bottomRight: Radius.circular(isUser ? 0 : 20),
                ),
                boxShadow: [
                  BoxShadow(
                    color: Colors.grey.withOpacity(0.18),
                    blurRadius: 5,
                    offset: const Offset(0, 3),
                  ),
                ],
              ),
              child: Text(
                text,
                style: TextStyle(
                  color: isUser ? Colors.white : Colors.black87,
                  fontSize: 16,
                ),
              ),
            ),
          ),
          if (isUser) const SizedBox(width: 10),
          if (isUser)
            const CircleAvatar(
              backgroundColor: Colors.deepPurple,
              child: Icon(Icons.person, color: Colors.white),
            ),
        ],
      ),
    );
  }
}
