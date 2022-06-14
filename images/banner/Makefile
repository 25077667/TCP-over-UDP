TEX = latexmk
BUILD_DIR = ./build
SRC = $(wildcard *.tex)

all: $(SRC)
	$(TEX) -pdf -output-directory=$(BUILD_DIR) $<

clean:
	$(RM) -r $(BUILD_DIR)
