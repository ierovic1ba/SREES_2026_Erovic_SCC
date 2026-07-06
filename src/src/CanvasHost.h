#pragma once
#include <gui/ViewScroller.h>

//
// Hosts a Canvas-derived view inside a gui::ViewScroller. A natID Canvas must be
// placed in a ViewScroller (its scroller parent) — putting a raw Canvas in a plain
// layout / TabView page corrupts the heap. Mirrors the SDK ScrolledCanvasView example.
//
template <class TCanvas>
class CanvasHost : public gui::ViewScroller
{
public:
	TCanvas _canvas;

	CanvasHost()
		: gui::ViewScroller(gui::ViewScroller::Type::ScrollAndAutoHide,
		                    gui::ViewScroller::Type::ScrollAndAutoHide)
	{
		setContentView(&_canvas);
	}

	// Detach the content view before the _canvas member is destroyed. _canvas is an
	// embedded member (not heap-allocated); if the ViewScroller base touches or frees
	// its content view during its own destruction, it hits an already-destroyed member
	// -> heap-corruption assertion on close. Clearing it here breaks that chain.
	~CanvasHost()
	{
		setContentView(nullptr);
	}
};
