# ==== Configuration =========================================================
NAME        := cocasse
CXX         ?= g++
STD         := -std=c++17
WARN        := -Wall -Wextra -Wpedantic
SRC_DIR     := .
BUILD_DIR   := build

SFML_VERSION := 3.1.0
SFML_REPO    := https://github.com/SFML/SFML.git
EXT_DIR      := external
SFML_SRC     := $(EXT_DIR)/SFML-src
SFML_DIR     := $(EXT_DIR)/sfml
SFML_LIBS    := sfml-graphics sfml-window sfml-system sfml-audio

# Sources : tous les .cpp du dossier (hors external/ et build/)
SRCS := $(shell find $(SRC_DIR) -name '*.cpp' -not -path './$(EXT_DIR)/*' -not -path './$(BUILD_DIR)/*')
OBJS := $(SRCS:%.cpp=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

# SFML local (external/sfml) si présent, sinon SFML système via pkg-config
ifneq ($(wildcard $(SFML_DIR)/lib),)
  SFML_CFLAGS := -I$(SFML_DIR)/include
  SFML_LDFLAGS := -L$(SFML_DIR)/lib -Wl,-rpath,'$$ORIGIN/$(SFML_DIR)/lib' $(addprefix -l,$(SFML_LIBS))
else
  SFML_CFLAGS := $(shell pkg-config --cflags $(SFML_LIBS) 2>/dev/null)
  SFML_LDFLAGS := $(shell pkg-config --libs $(SFML_LIBS) 2>/dev/null)
endif

# Mode : make MODE=release
MODE ?= debug
ifeq ($(MODE),release)
  OPT := -O2 -DNDEBUG
else
  OPT := -O0 -g3
endif

CXXFLAGS := $(STD) $(WARN) $(OPT) $(SFML_CFLAGS) -MMD -MP
LDFLAGS  := $(SFML_LDFLAGS)

# ==== Build =================================================================
.PHONY: all run release debug clean fclean re sfml sfml-clean format tidy \
        valgrind asan compile_commands info help

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

-include $(DEPS)

# ==== Utils =================================================================
run: $(NAME)                ## Compile et lance
	./$(NAME)

release:                    ## Build optimisé
	$(MAKE) fclean
	$(MAKE) MODE=release

debug:                      ## Build debug (défaut)
	$(MAKE) MODE=debug

asan:                       ## Build avec AddressSanitizer + UBSan
	$(MAKE) fclean
	$(MAKE) OPT="-O1 -g3 -fsanitize=address,undefined -fno-omit-frame-pointer" \
	        LDFLAGS="$(LDFLAGS) -fsanitize=address,undefined"

valgrind: $(NAME)           ## Lance sous valgrind
	valgrind --leak-check=full --show-leak-kinds=all ./$(NAME)

format:                     ## Formate les sources avec clang-format
	clang-format -i $(SRCS) $(shell find . -name '*.hpp' -not -path './$(EXT_DIR)/*')

tidy:                       ## Analyse statique avec clang-tidy
	clang-tidy $(SRCS) -- $(CXXFLAGS)

compile_commands:           ## Génère compile_commands.json (LSP/clangd)
	@echo '[' > compile_commands.json
	@first=1; for f in $(SRCS); do \
	  [ $$first -eq 1 ] || echo ',' >> compile_commands.json; first=0; \
	  printf '{"directory":"%s","file":"%s","command":"%s %s -c %s"}' \
	    "$(CURDIR)" "$$f" "$(CXX)" "$(CXXFLAGS)" "$$f" >> compile_commands.json; \
	done
	@echo ']' >> compile_commands.json
	@echo "compile_commands.json généré"

clean:                      ## Supprime les objets
	rm -rf $(BUILD_DIR)

fclean: clean               ## Supprime objets + binaire
	rm -f $(NAME)

re: fclean all              ## Recompile tout

# ==== SFML local ============================================================
sfml: $(SFML_DIR)/lib       ## Télécharge et compile SFML dans external/

$(SFML_DIR)/lib:
	@mkdir -p $(EXT_DIR)
	[ -d $(SFML_SRC) ] || git clone --depth 1 --branch $(SFML_VERSION) $(SFML_REPO) $(SFML_SRC)
	cmake -S $(SFML_SRC) -B $(SFML_SRC)/build \
	      -DCMAKE_BUILD_TYPE=Release \
	      -DCMAKE_INSTALL_PREFIX=$(CURDIR)/$(SFML_DIR) \
	      -DBUILD_SHARED_LIBS=ON \
	      -DSFML_BUILD_NETWORK=OFF -DSFML_BUILD_EXAMPLES=OFF -DSFML_BUILD_DOC=OFF -DSFML_BUILD_TEST_SUITE=OFF
	cmake --build $(SFML_SRC)/build -j$$(nproc)
	cmake --install $(SFML_SRC)/build
	@if [ -d $(SFML_DIR)/lib64 ] && [ ! -e $(SFML_DIR)/lib ]; then ln -s lib64 $(SFML_DIR)/lib; fi

sfml-clean:                 ## Supprime SFML local
	rm -rf $(EXT_DIR)

info:                       ## Affiche la config
	@echo "CXX      = $(CXX)"
	@echo "MODE     = $(MODE)"
	@echo "SRCS     = $(SRCS)"
	@echo "CXXFLAGS = $(CXXFLAGS)"
	@echo "LDFLAGS  = $(LDFLAGS)"

help:                       ## Affiche cette aide
	@grep -E '^[a-zA-Z_-]+:.*## ' $(MAKEFILE_LIST) | \
	  awk 'BEGIN{FS=":.*## "}{printf "  \033[36m%-18s\033[0m %s\n", $$1, $$2}'
