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
    : position(0), buffer(nullptr), size(0),
      prev(nullptr), next(nullptr)
{ }

TxtBuffer::TxtBuffer()
    : _buffer(nullptr), _bufferSize(0),
      _bufferAllocSize(0), _currentEvent(nullptr),
      _undoEventCount(0), _redoEventCount(0)
{
    _bufferAllocSize = TXT_BLOCK_SIZE;
    _buffer = new txtchr[_bufferAllocSize] { 0 };

    _firstEvent.buffer = new txtchr[1] { 0 };
    _firstEvent.next = nullptr;
    _firstEvent.position = 0;
    _firstEvent.prev = nullptr;
    _firstEvent.size = 0;
    _firstEvent.type = EditEventTypes::Insertion;

    _currentEvent = &_firstEvent;
}

void TxtBuffer::addEvent(EditEvent* event)
{
    auto remove = _currentEvent->next;
    while (remove != nullptr)
    {
        auto tmp = remove->next;
        delete []remove->buffer;
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

void TxtBuffer::addText(txtcur position, const txtchr* text, txtsz size)
{
    auto insertionEvent = new EditEvent();
    insertionEvent->buffer = new txtchr[size + 1] { 0 };
    copyString(insertionEvent->buffer, text, size);
    insertionEvent->next = nullptr;
    insertionEvent->position = position;
    insertionEvent->size = size;
    insertionEvent->type = EditEventTypes::Insertion;

    insertText(position, text, size);

    addEvent(insertionEvent);
}

void TxtBuffer::removeText(txtcur position, txtsz size)
{
    auto deletionEvent = new EditEvent();
    deletionEvent->buffer = new txtchr[size + 1] { 0 };
    copyString(deletionEvent->buffer, _buffer + position, size);
    deletionEvent->next = nullptr;
    deletionEvent->position = position;
    deletionEvent->size = size;
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
        deleteText(_currentEvent->position, _currentEvent->size);
    }
    else if (_currentEvent->type == EditEventTypes::Deletion)
    {
        insertText(_currentEvent->position, _currentEvent->buffer, _currentEvent->size);
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
        insertText(_currentEvent->position, _currentEvent->buffer, _currentEvent->size);
    }
    else if (_currentEvent->type == EditEventTypes::Deletion)
    {
        deleteText(_currentEvent->position, _currentEvent->size);
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
