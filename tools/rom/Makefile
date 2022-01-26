APP = mext
BUILD_PATH = build
CPPSRC = $(shell find src -name '*.cpp')
CSRC   = $(shell find src -name '*.c')
CPPOBJ := $(CPPSRC:%.cpp=%.o)
COBJ := $(CSRC:%.c=%.o)
OBJ := $(COBJ) $(CPPOBJ)
BUILTOBJ := $(addprefix $(BUILD_PATH)/,$(OBJ))
CINCLUDES = -I.
CPPINCLUDES = -Iinclude -I. `pkg-config --cflags python3`
LDLIBS ?= -lpython3.10

$(APP): $(BUILTOBJ)
	$(CXX) $(CPPFLAGS) -g -o $@ $(BUILTOBJ) $(LDLIBS)

$(BUILD_PATH)/%.o: %.cpp
	mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -g $(CPPINCLUDES) -c $< -o $@

$(BUILD_PATH)/%.o: %.c
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -g $(CINCLUDES) -c $< -o $@

clean:
	rm -rf $(BUILD_PATH)
	rm -f $(APP)