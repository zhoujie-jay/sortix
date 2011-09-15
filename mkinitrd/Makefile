CPPFLAGS=-I..
CXXFLAGS=-Wall

BINARIES=mkinitrd lsinitrd catinitrd

all: $(BINARIES)

%: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $< -o $@

clean:
	rm -f $(BINARIES)

install:
	
