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

    buffer.addText(0, "hello", 5);

    CHECK(buffer.undoCount() == 1);
    CHECK(buffer.redoCount() == 0);
    CHECK(std::string(buffer.buffer()) == std::string("hello"));

    buffer.addText(3, "olo", 3);

    CHECK(buffer.undoCount() == 2);
    CHECK(buffer.redoCount() == 0);
    CHECK(std::string(buffer.buffer()) == std::string("helololo"));
}

TEST_CASE("adding and removing text to an empty buffer should result in the correct text")
{
    TxtBuffer buffer;

    buffer.addText(0, "hello", 5);

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

    buffer.addText(0, "hello", 5);

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

    buffer.addText(0, "hello", 5);

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

    buffer.addText(0, "hello", 5);

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

    buffer.addText(0, "hello", 5);
    buffer.addText(3, "olo", 3);
    buffer.addText(8, "end", 3);

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

    buffer.addText(3, "olo", 3);

    CHECK(buffer.undoCount() == 2);
    CHECK(buffer.redoCount() == 0);
    CHECK(std::string(buffer.buffer()) == std::string("helololo"));
}
