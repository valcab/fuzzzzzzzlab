.PHONY: build debug release clean run-standalone

build:
	./scripts/build.sh Release

debug:
	./scripts/build.sh Debug

release:
	./scripts/build.sh Release

clean:
	rm -rf build

run-standalone: debug
	@echo "Launching standalone app..."
	@APP_PATH=$$(find build -type f -name "FuzzVST.app" | head -n 1); \
	if [ -z "$$APP_PATH" ]; then \
	  echo "Standalone app not found. Build may have failed."; \
	  exit 1; \
	fi; \
	open "$$APP_PATH"
