#include "txt.h"
#include <iostream>

#define TXT_BLOCK_SIZE 16

void printString(const txtchr* txt, txtsz size)
{
    std::cout << "|";
    for (int i = 0; i < size; i++)
    {
        if (txt[i] > ' ')
            std::cout << txt[i];
        else
            std::cout << '#';
    }
    std::cout << "|\n";
}

void copyString(txtchr* destination, const txtchr* source, txtsz size)
{
    size--;
    while (size >= 0)
    {
        destination[size] = source[size];
        size--;
    }
}

txtsz textSize(const txtchr* text)
{
    const txtchr* tmp = text;
    while (tmp[0] != '\0') tmp++;
    return tmp - text;
}

void moveTextUp(txtchr* buffer, txtsz bufferSize, txtcur position, txtsz size)
{
    bufferSize--;
    while (position <= bufferSize - size)
    {
        buffer[bufferSize] = buffer[bufferSize - size];
        bufferSize--;
    }
}

void moveTextDown(txtchr* buffer, txtsz size)
{
    do
    {
        buffer[0] = buffer[size];
        buffer++;
    }
    while (buffer[size] != '\0');
}

EditEvent::EditEvent()
    : position(0),
      prev(nullptr), next(nullptr)
{ }

TxtSelection::TxtSelection(TxtBuffer* txt)
    : _txt(txt), cursor(0), cursorLength(0)
{ }

void TxtSelection::addChar(txtchr c)
{
    if (c != '\0')
    {
        _txt->addText(*this, &c, 1);
        if (this->cursorLength < 0)
        {
            this->cursor += this->cursorLength;
        }
        this->cursor++;
        this->cursorLength = 0;
    }
}

void TxtSelection::addText(const txtchr* text)
{
    _txt->addText(*this, text, textSize(text));
    if (this->cursorLength < 0)
    {
        this->cursor += this->cursorLength;
    }
    this->cursor += textSize(text);
    this->cursorLength = 0;
}

void TxtSelection::moveLeft(bool shift, bool ctrl)
{
    if (this->cursor >= 0 && this->cursor + this->cursorLength > 0)
    {
        if (shift)
        {
            this->cursorLength--;
        }
        else
        {
            if (this->cursorLength != 0)
            {
                this->cursor += this->cursorLength;
            }
            else
            {
                this->cursor--;
            }
            this->cursorLength = 0;
        }
    }
}

void TxtSelection::moveRight(bool shift, bool ctrl)
{
    if (this->cursor < _txt->bufferSize() && this->cursor + this->cursorLength < _txt->bufferSize())
    {
        if (shift)
        {
            this->cursorLength++;
        }
        else
        {
            if (this->cursorLength != 0)
            {
                this->cursor += this->cursorLength;
            }
            else
            {
                this->cursor++;
            }
            this->cursorLength = 0;
        }
    }
}

void TxtSelection::moveUp(bool shift, bool ctrl)
{
    txtcur realCursor = cursor + cursorLength;

    auto lineStart = _txt->findLineStart(realCursor);
    auto diff = realCursor - lineStart;

    if (lineStart != 0)
    {
        auto prevLineStart = _txt->findLineStart(lineStart - 1);
        auto newCursor = prevLineStart + diff;

        if (_txt->findLineStart(newCursor) != prevLineStart)
        {
            newCursor = lineStart - 1;
        }

        if (!shift)
        {
            cursor = newCursor;
            cursorLength = 0;
        }
        else
        {
            cursorLength = newCursor - cursor;
        }
    }
}

void TxtSelection::moveDown(bool shift, bool ctrl)
{
    txtcur realCursor = cursor + cursorLength;

    auto lineStart = _txt->findLineStart(realCursor);
    auto nextLineStart = _txt->findNextLineStart(realCursor);

    if (nextLineStart >= 0)
    {
        auto newCursor = nextLineStart + (realCursor - lineStart);

        if (_txt->findLineStart(newCursor) != nextLineStart)
        {
            newCursor = _txt->findNextLineStart(nextLineStart) - 1;
        }

        if (!shift)
        {
            cursor = newCursor;
            this->cursorLength = 0;
        }
        else
        {
            cursorLength = newCursor - cursor;
        }
    }
}

void TxtSelection::selectAll()
{
    this->cursor = 0;
    this->cursorLength = _txt->bufferSize();
}

void TxtSelection::backspace(bool shift, bool ctrl)
{
    if (this->cursor > 0 || this->cursorLength > 0)
    {
        if (this->cursorLength == 0)
        {
            _txt->removeText(this->cursor - 1, 1);
            this->cursor--;
        }
        else
        {
            _txt->removeText(*this);
            if (this->cursorLength < 0)
            {
                this->cursor += this->cursorLength;
            }
            this->cursorLength = 0;
        }
    }
}

void TxtSelection::del(bool shift, bool ctrl)
{
    if (this->cursor < _txt->bufferSize())
    {
        if (this->cursorLength == 0)
        {
            _txt->removeText(this->cursor, 1);
        }
        else
        {
            _txt->removeText(*this);
            if (this->cursorLength < 0)
            {
                this->cursor += this->cursorLength;
            }
            this->cursorLength = 0;
        }
    }
}

void TxtSelection::home(bool shift, bool ctrl)
{
    cursor = ctrl ? 0 : _txt->findLineStart(cursor);
}

void TxtSelection::end(bool shift, bool ctrl)
{
    cursor = ctrl ? _txt->bufferSize() : _txt->findNextLineStart(cursor);

    if (cursor > 0 && _txt->buffer()[cursor-1] == '\n') cursor--;
}

TxtBuffer::TxtBuffer()
    : _buffer(nullptr), _bufferSize(0),
      _bufferAllocSize(0), _currentEvent(nullptr),
      _undoEventCount(0), _redoEventCount(0)
{
    _bufferAllocSize = TXT_BLOCK_SIZE;
    _buffer = new txtchr[_bufferAllocSize] { 0 };

    _firstEvent.buffer.clear();
    _firstEvent.next = nullptr;
    _firstEvent.position = 0;
    _firstEvent.prev = nullptr;
    _firstEvent.type = EditEventTypes::Insertion;

    _currentEvent = &_firstEvent;
}

void TxtBuffer::addEvent(EditEvent* event)
{
    auto remove = _currentEvent->next;
    while (remove != nullptr)
    {
        auto tmp = remove->next;
        delete remove;
        remove = tmp;
    }
    _redoEventCount = 0;

    event->prev = _currentEvent;
    _currentEvent->next = event;
    _currentEvent = event;
    _undoEventCount++;
}

void TxtBuffer::insertText(txtcur position, const txtchr* text, txtsz size)
{
    checkResize(_bufferSize + size);

    moveTextUp(_buffer, _bufferAllocSize, position, size);

    copyString(_buffer + position, text, size);

    _bufferSize += size;
}

void TxtBuffer::deleteText(txtcur position, txtsz size)
{
    moveTextDown(_buffer + position, size);

    _bufferSize -= size;

    _buffer[_bufferSize] = '\0';
}

void TxtBuffer::checkResize(txtsz size)
{
    if (_bufferAllocSize < size)
    {
        while (_bufferAllocSize < size)
        {
            _bufferAllocSize += TXT_BLOCK_SIZE;
        }
        auto newBuffer = new txtchr[_bufferAllocSize] { 0 };
        copyString(newBuffer, _buffer, _bufferSize);
        delete []_buffer;
        _buffer = newBuffer;
    }
}

void TxtBuffer::addText(const TxtSelection& selection, const txtchr* text, txtsz size)
{
    addText(selection.cursor, selection.cursorLength, text, size);
}

void TxtBuffer::addText(txtcur position, txtsz selectionLength, const txtchr* text, txtsz size)
{
    if (selectionLength != 0)
    {
        removeText(position, selectionLength);
        if (selectionLength < 0)
        {
            position += selectionLength;
        }
    }

    auto insertionEvent = new EditEvent();
    insertionEvent->buffer.assign(text, text + size);
    insertionEvent->next = nullptr;
    insertionEvent->position = position;
    insertionEvent->type = EditEventTypes::Insertion;

    insertText(position, text, size);

    addEvent(insertionEvent);
}

void TxtBuffer::removeText(const TxtSelection& selection)
{
    removeText(selection.cursor, selection.cursorLength);
}

void TxtBuffer::removeText(txtcur position, txtsz size)
{
    if (size < 0)
    {
        position = position + size;
        size = -size;
    }

    auto deletionEvent = new EditEvent();
    deletionEvent->buffer.assign(_buffer + position, _buffer + position + size);
    deletionEvent->next = nullptr;
    deletionEvent->position = position;
    deletionEvent->type = EditEventTypes::Deletion;

    deleteText(position, size);

    addEvent(deletionEvent);
}

bool TxtBuffer::undo()
{
    if (_currentEvent == &_firstEvent)
    {
        return false;
    }

    if (_currentEvent ->type == EditEventTypes::Insertion)
    {
        deleteText(_currentEvent->position, _currentEvent->buffer.size());
    }
    else if (_currentEvent->type == EditEventTypes::Deletion)
    {
        insertText(_currentEvent->position, _currentEvent->buffer.data(), _currentEvent->buffer.size());
    }

    _currentEvent = _currentEvent->prev;
    _undoEventCount--;
    _redoEventCount++;

    return true;
}

int TxtBuffer::undoCount()
{
    return _undoEventCount;
}

bool TxtBuffer::redo()
{
    if (_currentEvent->next == nullptr)
    {
        return false;
    }

    _currentEvent = _currentEvent->next;
    _undoEventCount++;
    _redoEventCount--;

    if (_currentEvent ->type == EditEventTypes::Insertion)
    {
        insertText(_currentEvent->position, _currentEvent->buffer.data(), _currentEvent->buffer.size());
    }
    else if (_currentEvent->type == EditEventTypes::Deletion)
    {
        deleteText(_currentEvent->position, _currentEvent->buffer.size());
    }

    return true;
}

int TxtBuffer::redoCount()
{
    return _redoEventCount;
}

const txtchr* TxtBuffer::buffer() const
{
    return this->_buffer;
}

const txtsz TxtBuffer::bufferSize() const
{
    return this->_bufferSize;
}

txtcur TxtBuffer::findLineStart(txtcur from) const
{
    if (from < 0 || from > _bufferSize) return -1;

    auto buffer = this->buffer();

    while (from > 0)
    {
        if (buffer[from - 1] == '\n') break;

        from--;
    }

    return from;
}

txtcur TxtBuffer::findNextLineStart(txtcur from) const
{
    if (from < 0 || from >= _bufferSize) return -1;

    auto buffer = this->buffer();

    while (from < this->bufferSize())
    {
        if (buffer[from] == '\n') return from + 1;

        from++;
    }

    return this->bufferSize();
}
