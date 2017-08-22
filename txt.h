#ifndef TXT_H
#define TXT_H

typedef long txtsz;     // size type, used for text buffer sizes
typedef long txtcur;    // cursor type, used for positions within text buffers
typedef char txtchr;    // char type, used as type of the text buffers

/*
 * --- Idea for editor data system ---
 * Every change is recorded in a doubly linked list, Such a record can
 * only be of type insertion or deletion. One method can generates the
 * full text for every recorded event. By saving the resulting buffer
 * size after such an event in the record, it is cheap to determine the
 * buffer size.
 */

enum class EditEventTypes
{
    Deletion,
    Insertion,
};

class EditEvent
{
public:
    EditEvent();

    EditEventTypes type;      // 0 means deletion, 1 means insertion
    txtcur position;              // the position in the main buffer to add or delete
    txtchr* buffer;                 // the added or removed text in this event
    txtsz size;                   // the size of the buffer text

    // double linked list
    EditEvent* prev;
    EditEvent* next;
};

class TxtBuffer
{
    txtchr* _buffer;
    txtsz _bufferSize;
    txtsz _bufferAllocSize;
    EditEvent _firstEvent;
    EditEvent* _currentEvent;
    int _undoEventCount;
    int _redoEventCount;

    void addEvent(EditEvent* event);
    void checkResize(txtsz size);

    void insertText(txtcur position, const txtchr* text, txtsz size);
    void deleteText(txtcur position, txtsz size);
public:
    TxtBuffer();

    void addText(txtcur position, const txtchr* text, txtsz size);
    void removeText(txtcur position, txtsz size);
    bool undo();
    int undoCount();
    bool redo();
    int redoCount();

    const char* buffer() const;
    const txtsz bufferSize() const;
};

#endif // TXT_H
