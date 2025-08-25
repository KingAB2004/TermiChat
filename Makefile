CXX = g++
CXXFLAGS = -Wall -std=c++17
LDLIBS = -lncurses -lsqlite3 -lssl -lcrypto

# Setup program sources
SETUP_SRC = Setup.cpp
SETUP_OBJ = $(SETUP_SRC:.cpp=.o)

# Main program sources
MAIN_SRC = Main.cpp \
           HandleSQL/AddFriends.cpp \
           HandleSQL/ListFriends.cpp \
           HandleUserName/UserName.cpp \
           Encryption/Encryptor.cpp \
           P2P_TermiChat/FilePicker.cpp \
           P2P_TermiChat/listner.cpp \
           P2P_TermiChat/Messages.cpp \
           P2P_TermiChat/PacketHandler.cpp \
           P2P_TermiChat/PrintMessage.cpp \
           P2P_TermiChat/Sender.cpp\
           P2P_TermiChat/ConnectionRequest.cpp\
           P2P_TermiChat/P2P_TermiChat.cpp


MAIN_OBJ = $(MAIN_SRC:.cpp=.o)

# Executable names
SETUP = Setup
MAIN = TermiChat

all: deps $(SETUP) $(MAIN)

deps:
	@command -v sqlite3 >/dev/null 2>&1 || { echo "Installing sqlite3..."; sudo apt-get update && sudo apt-get install -y sqlite3 libsqlite3-dev; }
	@command -v ncurses5-config >/dev/null 2>&1 || { echo "Installing ncurses..."; sudo apt-get install -y libncurses5-dev libncursesw5-dev; }
	@pkg-config --exists openssl || { echo "Installing openssl..."; sudo apt-get install -y libssl-dev; }

$(SETUP): $(SETUP_OBJ)
	$(CXX) $^ $(LDLIBS) -o $@

$(MAIN): $(MAIN_OBJ)
	$(CXX) $^ $(LDLIBS) -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(MAIN)
	./$(MAIN)

clean:
	rm -f $(SETUP_OBJ) $(MAIN_OBJ) $(SETUP) $(MAIN)
