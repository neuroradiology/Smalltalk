#ifndef ST_OBJECT_MEMORY_H
#define ST_OBJECT_MEMORY_H

/*
* Copyright 2020 Rochus Keller <mailto:me@rochus-keller.ch>
*
* This file is part of the Smalltalk parser/compiler library.
*
* The following is the license that applies to this copy of the
* library. For a license to use the library under conditions
* other than those described here, please email to me@rochus-keller.ch.
*
* GNU General Public License Usage
* This file may be used under the terms of the GNU General Public
* License (GPL) versions 2.0 or 3.0 as published by the Free Software
* Foundation and appearing in the file LICENSE.GPL included in
* the packaging of this file. Please review the following information
* to ensure GNU General Public Licensing requirements will be met:
* http://www.fsf.org/licensing/licenses/info/GPLv2.html and
* http://www.gnu.org/copyleft/gpl.html.
*/

#include <QObject>
#include <QSet>

class QIODevice;

namespace St
{
    class ObjectMemory : public QObject
    {
    public:
        // Objects known to the interpreter
        enum KnownObjects {
            // small integers
            objectMinusOne = 65535,
            objectZero = 1,
            objectOne = 3,
            objectTwo = 5,

            // undefined, boolean
            objectNil = 0x02,
            objectFalse = 0x04,
            objectTrue = 0x06,

            // root
            processor = 0x08, // an Association whose value field is Processor
            smalltalk = 0x12, // an Association whose value field is SystemDirectory

            // classes
            classSmallInteger = 0x0c,
            classString = 0x0e,
            classArray = 0x10,
            classFloat = 0x14,
            classMethodContext = 0x16,
            classBlockContext = 0x18,
            classPoint = 0x1a,
            classLargePositiveInteger = 0x1c,
            classDisplayBitmap = 0x1e,
            classMessage = 0x20,
            classCompiledMethod = 0x22,
            classSemaphore = 0x26,
            classCharacter = 0x28,

            // symbols
            symbolTable = 0x0a, // symbol class variable USTable
            symbolDoesNotUnderstand = 0x2a,
            symbolCannotReturn = 0x2c,
            symbolMonitor = 0x2e,
            symbolUnusedOop18 = 0x24,
            symbolMustBeBoolean = 0x34,

            // selectors
            specialSelectors = 0x30, // SystemDictionary class variable, the array of selectors for bytecodes 260-317 octal
            characterTable = 0x32, // Character class variable, table of characters

            // extra knowns
            classSymbol = 0x38,
        };

        enum CompiledMethodFlags {
            ZeroArguments = 0,
            OneArgument = 1,
            TwoArguments = 2,
            ThreeArguments = 3,
            FourArguments = 4,
            ZeroArgPrimitiveReturnSelf = 5, // no bytecode
            ZeroArgPrimitiveReturnVar = 6, // methodTemporaryCount returns index of the instance var to return, no bytecode
            HeaderExtension = 7
        };

        enum ClassIndizes {
            CI_superClass = 0,
            CI_messageDict = 1, // points to a MessageDictionary
            CI_instanceSpec = 2,
        };

        struct ByteString
        {
            const quint8* d_bytes;
            quint16 d_len;
            ByteString(const quint8* b, quint16 l):d_bytes(b),d_len(l){}
        };

        ObjectMemory(QObject* p = 0);
        bool readFrom( QIODevice* );

        QList<quint16> getAllValidOop() const;
        const QSet<quint16>& getObjects() const {return d_objects; }
        const QSet<quint16>& getClasses() const {return d_classes; }
        const QSet<quint16>& getMetaClasses() const {return d_metaClasses; }

        // oop 0 is reserved as an invalid object pointer!

        bool hasPointerMembers( quint16 objectPointer ) const;
        quint16 fetchPointerOfObject( quint16 fieldIndex, quint16 objectPointer ) const;
        void storePointerOfObject( quint16 fieldIndex, quint16 objectPointer, quint16 withValue );
        quint16 fetchWordOfObject( quint16 fieldIndex, quint16 objectPointer ) const;
        void storeWordOfObject( quint16 fieldIndex, quint16 objectPointer, quint16 withValue );
        quint8 fetchByteOfObject( quint16 byteIndex, quint16 objectPointer ) const;
        void storeByteOfObject( quint16 byteIndex, quint16 objectPointer, quint8 withValue );
        quint16 fetchClassOf( quint16 objectPointer ) const;
        quint16 fetchByteLenghtOf( quint16 objectPointer ) const;
        quint16 fetchWordLenghtOf( quint16 objectPointer ) const;
        ByteString fetchByteString( quint16 objectPointer ) const;

        quint16 instantiateClassWithPointers( quint16 classPointer, quint16 instanceSize );
        quint16 instantiateClassWithWords( quint16 classPointer, quint16 instanceSize );
        quint16 instantiateClassWithBytes( quint16 classPointer, quint16 instanceByteSize );
        QByteArray fetchClassName( quint16 classPointer ) const;

        quint8 methodTemporaryCount( quint16 methodPointer ) const; // including args
        CompiledMethodFlags methodFlags( quint16 methodPointer ) const;
        bool methodLargeContext( quint16 methodPointer ) const;
        quint8 methodLiteralCount( quint16 methodPointer ) const;
        ByteString methodBytecodes( quint16 methodPointer ) const;
        quint8 methodArgumentCount(quint16 methodPointer ) const;
        quint8 methodPrimitiveIndex(quint16 methodPointer ) const;
        quint16 methodLiteral(quint16 methodPointer, quint8 index ) const;
        // last literal contains Association to superclass in case of super call

        static bool isPointer(quint16);
        static qint16 toInt(quint16 objectPointer );

    protected:
        void printObjectTable();
        void printObjectSpace();
        quint32 getSpaceAddr(quint16 oop , bool* odd = 0, bool* ptr = 0) const;
        quint16 getClassOf( quint16 oop ) const;
        struct Data
        {
            quint32 d_pos;
            quint32 d_len : 31;
            quint32 d_isPtr : 1;
            Data():d_pos(0),d_len(0),d_isPtr(false){}
        };
        Data getDataOf(quint16 oop , bool noHeader = true ) const;
        qint32 findNextFree();
        quint16 createInstance(quint16 classPtr, quint16 byteLen, bool isPtr );

    private:
        QByteArray d_objectSpace; // first word is oop_2; contains no unused/free space
        QByteArray d_objectTable; // first word is oop_0; entries are oop or unused (freeEntry bit set)
        // objects are stored continuously, no segment boundaries
        // all objectTable entries are the same size
        QSet<quint16> d_objects, d_classes, d_metaClasses;
    };
}

#endif // ST_OBJECT_MEMORY_H
