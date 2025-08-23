CXX = g++
CXXFLAGS = -Wall -std=c++17 -lncurses -lsqlite3 -lssl -lcrypto

# Source files
SRC = Main.cpp \
      P2P_TermiChat/P2P_TermiChat.cpp \
      HandleSQL/AddFriends.cpp \
      HandleSQL/ListFriends.cpp \
      HandleUserName/UserName.cpp \
      Encryption/Encryptor.cpp \
      Setup.cpp

# Object files
OBJ = $(SRC:.cpp=.o)

# Executable names
SETUP = Setup
MAIN = TermiChat

all: $(SETUP) $(MAIN)

$(SETUP): Setup.o
	$(CXX) $^ -lsqlite3 -o $@

$(MAIN): Main.o P2P_TermiChat/P2P_TermiChat.o HandleSQL/AddFriends.o HandleSQL/ListFriends.o HandleUserName/UserName.o Encryption/Encryptor.o
	$(CXX) $^ $(CXXFLAGS) -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(MAIN)
	./$(MAIN)

clean:
	rm -f $(OBJ) $(SETUP) $(MAIN)
