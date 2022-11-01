#https://www.gnu.org/software/make/manual/html_node/Implicit-Variables.html

CC  = g++
CXX = g++
LD  = g++

RM = rm -rf

OPTIM_FLAGS = -O3
GTEST_FLAGS = -pthread -g
OTHER_FLAGS = -MD

FLAGS  = $(OPTIM_FLAGS)

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