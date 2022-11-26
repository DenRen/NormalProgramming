#https://www.gnu.org/software/make/manual/html_node/Implicit-Variables.html

CXX = clang++-14
CC  = $(CXX)
LD  = $(CXX)

RM = rm -rf

OPTIM_FLAGS = -O3
OTHER_FLAGS = -MD -pthread -std=c++17

FLAGS  = $(OPTIM_FLAGS) $(OTHER_FLAGS)

CFLAGS 	 = $(FLAGS)
CXXFLAGS = $(FLAGS)
LDFLAGS  = $(FLAGS)

OBJS = main.o

# Main -------------------------

all: a.out

a.out: $(OBJS)
	$(LD) $(CFLAGS) -o $@ $^

clean:
	$(RM) a.out $(OBJS) *.gcno *.gcda *.info *.d

-include $(OBJS:.o=.d)

.PHONY: clean all