//
//  SREES 2026 — Proracun kratkih spojeva za transmisione mreze (simetricne mreze)
//  Short-circuit calculation for symmetrical transmission networks.
//
//  Standalone natID GUI application. Entry point.
//
#include "Application.h"
#include <td/StringConverter.h>
#include <gui/WinMain.h>
#include <gui/Application.h>
#include <fo/FileOperations.h>
#include <system_error>

int main(int argc, const char* argv[])
{
	Application app(argc, argv);
	app.init("BA"); // change to "EN" for English

	// Start file dialogs in the bundled test-case folder (derived from the resource
	// path, so it stays portable). The open dialog otherwise remembers the last folder.
	std::error_code ec;
	fo::fs::path resPath = gui::getApplication()->getResPath();
	fo::fs::path testcases = resPath.parent_path() / "testcases";
	if (fo::fs::exists(testcases, ec))
		fo::fs::current_path(testcases, ec);

	return app.run();
}
