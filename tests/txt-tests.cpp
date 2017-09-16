#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "../txt.h"
#include <string>

TEST_CASE("default constructor should result in an empty buffer")
{
    TxtBuffer buffer;

    CHECK(buffer.buffer()[0] == '\0');
    CHECK(buffer.bufferSize() == 0);
    CHECK(buffer.undoCount() == 0);
    CHECK(buffer.redoCount() == 0);
}

TEST_CASE("adding text twice to an empty buffer should result in a buffer containing the added text")
{
    TxtBuffer buffer;

    buffer.addText(0, 0, "hello", 5);

    CHECK(buffer.undoCount() == 1);
    CHECK(buffer.redoCount() == 0);
    CHECK(std::string(buffer.buffer()) == std::string("hello"));

    buffer.addText(3, 0, "olo", 3);

    CHECK(buffer.undoCount() == 2);
    CHECK(buffer.redoCount() == 0);
    CHECK(std::string(buffer.buffer()) == std::string("helololo"));
}

TEST_CASE("adding and removing text to an empty buffer should result in the correct text")
{
    TxtBuffer buffer;

    buffer.addText(0, 0, "hello", 5);

    CHECK(buffer.undoCount() == 1);
    CHECK(buffer.redoCount() == 0);
    CHECK(std::string(buffer.buffer()) == std::string("hello"));

    buffer.removeText(2, 2);

    CHECK(buffer.undoCount() == 2);
    CHECK(buffer.redoCount() == 0);
    CHECK(std::string(buffer.buffer()) == std::string("heo"));
}

TEST_CASE("adding a text and removing all text to an empty buffer should result in an empty buffer")
{
    TxtBuffer buffer;

    buffer.addText(0, 0, "hello", 5);

    CHECK(buffer.undoCount() == 1);
    CHECK(buffer.redoCount() == 0);
    CHECK(std::string(buffer.buffer()) == std::string("hello"));

    buffer.removeText(0, 5);

    CHECK(buffer.undoCount() == 2);
    CHECK(buffer.redoCount() == 0);
    CHECK(std::string(buffer.buffer()) == std::string(""));
}

TEST_CASE("adding a text to an empty buffer and undoing should result in an empty buffer")
{
    TxtBuffer buffer;

    buffer.addText(0, 0, "hello", 5);

    CHECK(buffer.undoCount() == 1);
    CHECK(buffer.redoCount() == 0);
    CHECK(std::string(buffer.buffer()) == std::string("hello"));

    CHECK(buffer.undo() == true);

    CHECK(buffer.undoCount() == 0);
    CHECK(buffer.redoCount() == 1);
    CHECK(std::string(buffer.buffer()) == std::string(""));
}

TEST_CASE("adding a text to an empty buffer, undoing and redoing should result in the added text")
{
    TxtBuffer buffer;

    buffer.addText(0, 0, "hello", 5);

    CHECK(buffer.undoCount() == 1);
    CHECK(buffer.redoCount() == 0);
    CHECK(std::string(buffer.buffer()) == std::string("hello"));

    CHECK(buffer.undo() == true);

    CHECK(buffer.undoCount() == 0);
    CHECK(buffer.redoCount() == 1);
    CHECK(std::string(buffer.buffer()) == std::string(""));

    CHECK(buffer.redo() == true);

    CHECK(buffer.undoCount() == 1);
    CHECK(buffer.redoCount() == 0);
    CHECK(std::string(buffer.buffer()) == std::string("hello"));

    CHECK(buffer.redo() == false);
}

TEST_CASE("adding a text to an empty buffer, undoing and adding text again should reset redo count")
{
    TxtBuffer buffer;

    buffer.addText(0, 0, "hello", 5);
    buffer.addText(3, 0, "olo", 3);
    buffer.addText(8, 0, "end", 3);

    CHECK(buffer.undoCount() == 3);
    CHECK(buffer.redoCount() == 0);
    CHECK(std::string(buffer.buffer()) == std::string("helololoend"));

    CHECK(buffer.undo() == true);

    CHECK(buffer.undoCount() == 2);
    CHECK(buffer.redoCount() == 1);
    CHECK(std::string(buffer.buffer()) == std::string("helololo"));

    CHECK(buffer.undo() == true);

    CHECK(buffer.undoCount() == 1);
    CHECK(buffer.redoCount() == 2);
    CHECK(std::string(buffer.buffer()) == std::string("hello"));

    buffer.addText(3, 0, "olo", 3);

    CHECK(buffer.undoCount() == 2);
    CHECK(buffer.redoCount() == 0);
    CHECK(std::string(buffer.buffer()) == std::string("helololo"));
}

TEST_CASE("adding a text over a selection range should work")
{
    TxtBuffer buffer;

    buffer.addText(0, 0, "hello", 5);
    buffer.addText(2, 2, "rr", 2);

    CHECK(std::string(buffer.buffer()) == std::string("herro"));
}

TEST_CASE("adding a text over a negative selection range should work")
{
    TxtBuffer buffer;

    buffer.addText(0, 0, "hello", 5);
    buffer.addText(4, -2, "rr", 2);

    CHECK(std::string(buffer.buffer()) == std::string("herro"));
}

TEST_CASE("findLineStart should work")
{
    TxtBuffer buffer;

    buffer.addText(0, 0, "hello\ntest\nbla", 15);

    CHECK(buffer.findLineStart(0) == 0);
    CHECK(buffer.findLineStart(2) == 0);
    CHECK(buffer.findLineStart(4) == 0);
    CHECK(buffer.findLineStart(5) == 5);
    CHECK(buffer.findLineStart(6) == 5);
    CHECK(buffer.findLineStart(7) == 5);
    CHECK(buffer.findLineStart(8) == 5);
    CHECK(buffer.findLineStart(9) == 5);
    CHECK(buffer.findLineStart(10) == 10);
    CHECK(buffer.findLineStart(11) == 10);
    CHECK(buffer.findLineStart(12) == 10);
    CHECK(buffer.findLineStart(13) == 10);
    CHECK(buffer.findLineStart(14) == 10);
    CHECK(buffer.findLineStart(15) == 10);
}

TEST_CASE("findLineStart with a position outside the text should return -1")
{
    TxtBuffer buffer;

    buffer.addText(0, 0, "hello\ntest\nbla", 15);

    CHECK(buffer.findLineStart(16) == -1);
}

TEST_CASE("findLineStart with a negative position should return -1")
{
    TxtBuffer buffer;

    buffer.addText(0, 0, "hello\ntest\nbla", 15);

    CHECK(buffer.findLineStart(-2) == -1);
}

TEST_CASE("findNextLineStart should work")
{
    TxtBuffer buffer;

    buffer.addText(0, 0, "hello\ntest\nbla", 15);

    CHECK(buffer.findNextLineStart(0) == 6);
    CHECK(buffer.findNextLineStart(2) == 6);
    CHECK(buffer.findNextLineStart(4) == 6);
    CHECK(buffer.findNextLineStart(5) == 6);
    CHECK(buffer.findNextLineStart(6) == 11);
    CHECK(buffer.findNextLineStart(7) == 11);
    CHECK(buffer.findNextLineStart(8) == 11);
    CHECK(buffer.findNextLineStart(9) == 11);
    CHECK(buffer.findNextLineStart(10) == 11);
    CHECK(buffer.findNextLineStart(11) == -1);
    CHECK(buffer.findNextLineStart(12) == -1);
    CHECK(buffer.findNextLineStart(13) == -1);
    CHECK(buffer.findNextLineStart(14) == -1);
    CHECK(buffer.findNextLineStart(15) == -1);
}

TEST_CASE("findNextLineStart with a position outside the text should return -1")
{
    TxtBuffer buffer;

    buffer.addText(0, 0, "hello\ntest\nbla", 15);

    CHECK(buffer.findNextLineStart(16) == -1);
}

TEST_CASE("findNextLineStart with a negative position should return -1")
{
    TxtBuffer buffer;

    buffer.addText(0, 0, "hello\ntest\nbla", 15);

    CHECK(buffer.findNextLineStart(-2) == -1);
}
