/*
 * Copyright 2013 Real Logic Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "gtest/gtest.h"
#include "otf_api/Listener.h"

using namespace sbe::on_the_fly;
using ::std::cout;
using ::std::endl;

#include "OtfMessageTest.h"
#include "OtfMessageTestCBs.h"

class OtfMessageTestMessage : public OtfMessageTestCBs
{
public:
    virtual int onNext(const Field &f)
    {
        OtfMessageTestCBs::onNext(f);

        if (numFieldsSeen_ == 1)
        {
            EXPECT_EQ(f.type(), Field::COMPOSITE);
            EXPECT_EQ(f.fieldName(), "");
            EXPECT_EQ(f.compositeName(), "messageHeader");
            EXPECT_EQ(f.schemaId(), Field::INVALID_ID);
            EXPECT_EQ(f.numEncodings(), 4);
            EXPECT_EQ(f.encodingName(0), "blockLength");
            EXPECT_EQ(f.encodingName(1), "templateId");
            EXPECT_EQ(f.encodingName(2), "version");
            EXPECT_EQ(f.encodingName(3), "reserved");
            EXPECT_EQ(f.primitiveType(0), Ir::UINT16);
            EXPECT_EQ(f.primitiveType(1), Ir::UINT16);
            EXPECT_EQ(f.primitiveType(2), Ir::UINT8);
            EXPECT_EQ(f.primitiveType(3), Ir::UINT8);
            EXPECT_EQ(f.getUInt(0), BLOCKLENGTH);
            EXPECT_EQ(f.getUInt(1), TEMPLATE_ID);
            EXPECT_EQ(f.getUInt(2), VERSION);
            EXPECT_EQ(f.getUInt(3), 0);
        }
        else if (numFieldsSeen_ == 2)
        {
            EXPECT_EQ(f.type(), Field::ENCODING);
            EXPECT_EQ(f.fieldName(), "Field1");
            EXPECT_EQ(f.schemaId(), FIELD_ID);
            EXPECT_EQ(f.numEncodings(), 1);
            EXPECT_EQ(f.encodingName(0), "uint32");
            EXPECT_EQ(f.primitiveType(0), Ir::UINT32);
            EXPECT_EQ(f.getUInt(0), FIELD_VALUE);
        }
        return 0;
    };
};

TEST_F(OtfMessageTest, shouldHandleMessageDispatch)
{
    OtfMessageTestMessage cbs;

    listener_.dispatchMessageByHeader(std::string("templateId"), messageHeaderIr_, this)
        .resetForDecode(buffer_, bufferLen_)
        .subscribe(&cbs, &cbs);
    EXPECT_EQ(cbs.numFieldsSeen_, 2);
    EXPECT_EQ(cbs.numErrorsSeen_, 0);
    EXPECT_EQ(cbs.numCompletedsSeen_, 0);
}

TEST_F(OtfMessageTest, shouldHandleMessageDispatchWithOnCompleted)
{
    OtfMessageTestMessage cbs;

    listener_.dispatchMessageByHeader(std::string("templateId"), messageHeaderIr_, this)
        .resetForDecode(buffer_, bufferLen_)
        .subscribe(&cbs, &cbs, &cbs);
    EXPECT_EQ(cbs.numFieldsSeen_, 2);
    EXPECT_EQ(cbs.numErrorsSeen_, 0);
    EXPECT_EQ(cbs.numCompletedsSeen_, 1);
}

TEST_F(OtfMessageTest, shouldHandleMessageDispatchWithTooShortMessage)
{
    OtfMessageTestMessage cbs;

    bufferLen_ -= 2;
    listener_.dispatchMessageByHeader(std::string("templateId"), messageHeaderIr_, this)
        .resetForDecode(buffer_, bufferLen_)
        .subscribe(&cbs, &cbs, &cbs);
    EXPECT_EQ(cbs.numFieldsSeen_, 1);
    EXPECT_EQ(cbs.numErrorsSeen_, 1);
    EXPECT_EQ(cbs.numCompletedsSeen_, 0);
}

TEST_F(OtfMessageTest, shouldHandleMessageDispatchWithTooShortMessageHeader)
{
    OtfMessageTestMessage cbs;

    bufferLen_ -= 5;
    listener_.dispatchMessageByHeader(std::string("templateId"), messageHeaderIr_, this)
        .resetForDecode(buffer_, bufferLen_)
        .subscribe(&cbs, &cbs, &cbs);
    EXPECT_EQ(cbs.numFieldsSeen_, 0);
    EXPECT_EQ(cbs.numErrorsSeen_, 1);
    EXPECT_EQ(cbs.numCompletedsSeen_, 0);
}

TEST_F(OtfMessageTest, shouldHandleMessageDispatchWithNoRoomForMessage)
{
    OtfMessageTestMessage cbs;

    bufferLen_ -= sizeof(uint32_t);
    listener_.dispatchMessageByHeader(std::string("templateId"), messageHeaderIr_, this)
        .resetForDecode(buffer_, bufferLen_)
        .subscribe(&cbs, &cbs, &cbs);
    EXPECT_EQ(cbs.numFieldsSeen_, 1);
    EXPECT_EQ(cbs.numErrorsSeen_, 1);
    EXPECT_EQ(cbs.numCompletedsSeen_, 0);
}

TEST_F(OtfMessageTest, shouldHandleMessageDispatchWithNoTemplateIDEncodingName)
{
    OtfMessageTestMessage cbs;

    listener_.dispatchMessageByHeader(std::string("notGoingToBeFound!"), messageHeaderIr_, this)
        .resetForDecode(buffer_, bufferLen_)
        .subscribe(&cbs, &cbs, &cbs);
    EXPECT_EQ(cbs.numFieldsSeen_, 1);
    EXPECT_EQ(cbs.numErrorsSeen_, 1);
    EXPECT_EQ(cbs.numCompletedsSeen_, 0);
}

class OtfMessageTestIrCallbackNULL : public Ir::Callback
{
public:
    virtual Ir *irForTemplateId(const int templateId)
    {
        return NULL;
    };
};

TEST_F(OtfMessageTest, shouldHandleMessageDispatchWithIrCallbackReturningNULL)
{
    OtfMessageTestMessage cbs;
    OtfMessageTestIrCallbackNULL ircb;

    listener_.dispatchMessageByHeader(std::string("templateId"), messageHeaderIr_, &ircb)
        .resetForDecode(buffer_, bufferLen_)
        .subscribe(&cbs, &cbs, &cbs);
    EXPECT_EQ(cbs.numFieldsSeen_, 1);
    EXPECT_EQ(cbs.numErrorsSeen_, 1);
    EXPECT_EQ(cbs.numCompletedsSeen_, 0);
}

class OtfMessageAllPrimitiveTypesTest : public OtfMessageTest, public OtfMessageTestCBs
{
protected:
#define FIELD_CHAR_VALUE 0xB
#define FIELD_INT8_VALUE -0xB
#define FIELD_INT16_VALUE -0x0EED
#define FIELD_INT32_VALUE -0x0EEDBEEF
#define FIELD_INT64_VALUE -0x0EEDFEEDBEEFL
#define FIELD_UINT8_VALUE 0xB
#define FIELD_UINT16_VALUE 0xDEED
#define FIELD_UINT32_VALUE 0xFEEDBEEF
#define FIELD_UINT64_VALUE 0xDEEDFEEDBEEFL
#define FIELD_FLOAT_VALUE 7.11f
#define FIELD_DOUBLE_VALUE 711.711f

    virtual void constructMessageIr(Ir &ir)
    {
        Ir::TokenByteOrder byteOrder = Ir::SBE_LITTLE_ENDIAN;
        std::string messageStr = std::string("Message1");
        std::string fieldStr = std::string("Field1");
        std::string compositeStr = std::string("AllTypes");

        ir.addToken(0, 43, Ir::BEGIN_MESSAGE, byteOrder, Ir::NONE, TEMPLATE_ID, messageStr);
        ir.addToken(0, 0, Ir::BEGIN_FIELD, byteOrder, Ir::NONE, FIELD_ID, fieldStr);
        ir.addToken(0, 0, Ir::BEGIN_COMPOSITE, byteOrder, Ir::NONE, 0xFFFF, compositeStr);
        ir.addToken(0, 1, Ir::ENCODING, byteOrder, Ir::CHAR, 0xFFFF, std::string("char"));
        ir.addToken(1, 1, Ir::ENCODING, byteOrder, Ir::INT8, 0xFFFF, std::string("int8"));
        ir.addToken(2, 2, Ir::ENCODING, byteOrder, Ir::INT16, 0xFFFF, std::string("int16"));
        ir.addToken(4, 4, Ir::ENCODING, byteOrder, Ir::INT32, 0xFFFF, std::string("int32"));
        ir.addToken(8, 8, Ir::ENCODING, byteOrder, Ir::INT64, 0xFFFF, std::string("int64"));
        ir.addToken(16, 1, Ir::ENCODING, byteOrder, Ir::UINT8, 0xFFFF, std::string("uint8"));
        ir.addToken(17, 2, Ir::ENCODING, byteOrder, Ir::UINT16, 0xFFFF, std::string("uint16"));
        ir.addToken(19, 4, Ir::ENCODING, byteOrder, Ir::UINT32, 0xFFFF, std::string("uint32"));
        ir.addToken(23, 8, Ir::ENCODING, byteOrder, Ir::UINT64, 0xFFFF, std::string("uint64"));
        ir.addToken(31, 4, Ir::ENCODING, byteOrder, Ir::FLOAT, 0xFFFF, std::string("float"));
        ir.addToken(35, 8, Ir::ENCODING, byteOrder, Ir::DOUBLE, 0xFFFF, std::string("double"));
        ir.addToken(0, 0, Ir::END_COMPOSITE, byteOrder, Ir::NONE, 0xFFFF, compositeStr);
        ir.addToken(0, 0, Ir::END_FIELD, byteOrder, Ir::NONE, FIELD_ID, fieldStr);
        ir.addToken(0, 43, Ir::END_MESSAGE, byteOrder, Ir::NONE, TEMPLATE_ID, messageStr);
    };

    virtual void constructMessage()
    {
        *((char *)(buffer_ + bufferLen_)) = FIELD_CHAR_VALUE;
        bufferLen_ += sizeof(char);

        *((int8_t *)(buffer_ + bufferLen_)) = FIELD_INT8_VALUE;
        bufferLen_ += sizeof(int8_t);

        *((int16_t *)(buffer_ + bufferLen_)) = FIELD_INT16_VALUE;
        bufferLen_ += sizeof(int16_t);

        *((int32_t *)(buffer_ + bufferLen_)) = FIELD_INT32_VALUE;
        bufferLen_ += sizeof(int32_t);

        *((int64_t *)(buffer_ + bufferLen_)) = FIELD_INT64_VALUE;
        bufferLen_ += sizeof(int64_t);

        *((uint8_t *)(buffer_ + bufferLen_)) = FIELD_UINT8_VALUE;
        bufferLen_ += sizeof(uint8_t);

        *((uint16_t *)(buffer_ + bufferLen_)) = FIELD_UINT16_VALUE;
        bufferLen_ += sizeof(uint16_t);

        *((uint32_t *)(buffer_ + bufferLen_)) = FIELD_UINT32_VALUE;
        bufferLen_ += sizeof(uint32_t);

        *((uint64_t *)(buffer_ + bufferLen_)) = FIELD_UINT64_VALUE;
        bufferLen_ += sizeof(uint64_t);

        *((float *)(buffer_ + bufferLen_)) = FIELD_FLOAT_VALUE;
        bufferLen_ += sizeof(float);

        *((double *)(buffer_ + bufferLen_)) = FIELD_DOUBLE_VALUE;
        bufferLen_ += sizeof(double);
    };

    virtual int onNext(const Field &f)
    {
        OtfMessageTestCBs::onNext(f);

        if (numFieldsSeen_ == 2)
        {
            EXPECT_EQ(f.numEncodings(), 11);
            EXPECT_EQ(f.primitiveType(0), Ir::CHAR);
            EXPECT_EQ(f.primitiveType(1), Ir::INT8);
            EXPECT_EQ(f.primitiveType(2), Ir::INT16);
            EXPECT_EQ(f.primitiveType(3), Ir::INT32);
            EXPECT_EQ(f.primitiveType(4), Ir::INT64);
            EXPECT_EQ(f.primitiveType(5), Ir::UINT8);
            EXPECT_EQ(f.primitiveType(6), Ir::UINT16);
            EXPECT_EQ(f.primitiveType(7), Ir::UINT32);
            EXPECT_EQ(f.primitiveType(8), Ir::UINT64);
            EXPECT_EQ(f.primitiveType(9), Ir::FLOAT);
            EXPECT_EQ(f.primitiveType(10), Ir::DOUBLE);
            EXPECT_EQ(f.getInt(0), FIELD_CHAR_VALUE);
            EXPECT_EQ(f.getInt(1), FIELD_INT8_VALUE);
            EXPECT_EQ(f.getInt(2), FIELD_INT16_VALUE);
            EXPECT_EQ(f.getInt(3), FIELD_INT32_VALUE);
            EXPECT_EQ(f.getInt(4), FIELD_INT64_VALUE);
            EXPECT_EQ(f.getUInt(5), FIELD_UINT8_VALUE);
            EXPECT_EQ(f.getUInt(6), FIELD_UINT16_VALUE);
            EXPECT_EQ(f.getUInt(7), FIELD_UINT32_VALUE);
            EXPECT_EQ(f.getUInt(8), FIELD_UINT64_VALUE);
            EXPECT_EQ(f.getDouble(9), FIELD_FLOAT_VALUE);
            EXPECT_EQ(f.getDouble(10), FIELD_DOUBLE_VALUE);
        }
        return 0;
    };
};

TEST_F(OtfMessageAllPrimitiveTypesTest, shouldHandleAllTypes)
{
    listener_.dispatchMessageByHeader(std::string("templateId"), messageHeaderIr_, this)
        .resetForDecode(buffer_, bufferLen_)
        .subscribe(this, this, this);
    EXPECT_EQ(numFieldsSeen_, 2);
    EXPECT_EQ(numErrorsSeen_, 0);
    EXPECT_EQ(numCompletedsSeen_, 1);
}

class OtfMessageEnumTest : public OtfMessageTest, public OtfMessageTestCBs
{
protected:
#define FIELD_ENUM_CHAR_VALUE 0x31
#define FIELD_ENUM_UINT8_VALUE 0x10

    virtual void constructMessageIr(Ir &ir)
    {
        Ir::TokenByteOrder byteOrder = Ir::SBE_LITTLE_ENDIAN;
        std::string messageStr = std::string("MessageWithEnum");
        std::string charFieldStr = std::string("EnumCHARField");
        std::string uint8FieldStr = std::string("EnumUINT8Field");
        std::string noValidValueFieldStr = std::string("EnumNoValidValueField");

        ir.addToken(0, 3, Ir::BEGIN_MESSAGE, byteOrder, Ir::NONE, TEMPLATE_ID, messageStr);
        ir.addToken(0, 0, Ir::BEGIN_FIELD, byteOrder, Ir::NONE, FIELD_ID, charFieldStr);
        ir.addToken(0, 1, Ir::BEGIN_ENUM, byteOrder, Ir::CHAR, 0xFFFF, std::string("char"));
        ir.addToken(0, 0, Ir::VALID_VALUE, byteOrder, Ir::CHAR, 0x31, std::string("charValue1"));
        ir.addToken(0, 0, Ir::VALID_VALUE, byteOrder, Ir::CHAR, 0x32, std::string("charValue2"));
        ir.addToken(0, 0, Ir::END_ENUM, byteOrder, Ir::NONE, 0xFFFF, std::string("char"));
        ir.addToken(0, 0, Ir::END_FIELD, byteOrder, Ir::NONE, FIELD_ID, charFieldStr);
        ir.addToken(0, 0, Ir::BEGIN_FIELD, byteOrder, Ir::NONE, FIELD_ID + 1, uint8FieldStr);
        ir.addToken(1, 1, Ir::BEGIN_ENUM, byteOrder, Ir::UINT8, 0xFFFF, std::string("uint8"));
        ir.addToken(0, 0, Ir::VALID_VALUE, byteOrder, Ir::UINT8, 0x09, std::string("uint8Value1"));
        ir.addToken(0, 0, Ir::VALID_VALUE, byteOrder, Ir::UINT8, 0x10, std::string("uint8Value2"));
        ir.addToken(0, 0, Ir::END_ENUM, byteOrder, Ir::NONE, 0xFFFF, std::string("uint8"));
        ir.addToken(0, 0, Ir::END_FIELD, byteOrder, Ir::NONE, FIELD_ID + 1, uint8FieldStr);
        ir.addToken(0, 0, Ir::BEGIN_FIELD, byteOrder, Ir::NONE, FIELD_ID + 2, noValidValueFieldStr);
        ir.addToken(2, 1, Ir::BEGIN_ENUM, byteOrder, Ir::UINT8, 0xFFFF, std::string("uint8"));
        ir.addToken(0, 0, Ir::VALID_VALUE, byteOrder, Ir::UINT8, 0x09, std::string("uint8Value1"));
        ir.addToken(0, 0, Ir::VALID_VALUE, byteOrder, Ir::UINT8, 0x10, std::string("uint8Value2"));
        ir.addToken(0, 0, Ir::END_ENUM, byteOrder, Ir::NONE, 0xFFFF, std::string("uint8"));
        ir.addToken(0, 0, Ir::END_FIELD, byteOrder, Ir::NONE, FIELD_ID + 2, noValidValueFieldStr);
        ir.addToken(0, 3, Ir::END_MESSAGE, byteOrder, Ir::NONE, TEMPLATE_ID, messageStr);
    };

    virtual void constructMessage()
    {
        *((char *)(buffer_ + bufferLen_)) = FIELD_ENUM_CHAR_VALUE;
        bufferLen_ += sizeof(char);

        *((uint8_t *)(buffer_ + bufferLen_)) = FIELD_ENUM_UINT8_VALUE;
        bufferLen_ += sizeof(uint8_t);

        *((uint8_t *)(buffer_ + bufferLen_)) = 0;
        bufferLen_ += sizeof(uint8_t);
    };

    virtual int onNext(const Field &f)
    {
        OtfMessageTestCBs::onNext(f);

        if (numFieldsSeen_ == 2)
        {
            EXPECT_EQ(f.type(), Field::ENUM);
            EXPECT_EQ(f.numEncodings(), 1);
            EXPECT_EQ(f.fieldName(), "EnumCHARField");
            EXPECT_EQ(f.primitiveType(), Ir::CHAR);
            EXPECT_EQ(f.getUInt(), FIELD_ENUM_CHAR_VALUE);
            EXPECT_EQ(f.validValue(), "charValue1");
        }
        else if (numFieldsSeen_ == 3)
        {
            EXPECT_EQ(f.type(), Field::ENUM);
            EXPECT_EQ(f.numEncodings(), 1);
            EXPECT_EQ(f.fieldName(), "EnumUINT8Field");
            EXPECT_EQ(f.primitiveType(), Ir::UINT8);
            EXPECT_EQ(f.getUInt(), FIELD_ENUM_UINT8_VALUE);
            EXPECT_EQ(f.validValue(), "uint8Value2");
        }
        else if (numFieldsSeen_ == 3)
        {
            EXPECT_EQ(f.type(), Field::ENUM);
            EXPECT_EQ(f.numEncodings(), 1);
            EXPECT_EQ(f.fieldName(), "EnumNoValidValueField");
            EXPECT_EQ(f.primitiveType(), Ir::UINT8);
            EXPECT_EQ(f.getUInt(), 0);
            EXPECT_EQ(f.validValue(), "");
        }
        return 0;
    };
};

TEST_F(OtfMessageEnumTest, shouldHandleEnum)
{
    listener_.dispatchMessageByHeader(std::string("templateId"), messageHeaderIr_, this)
        .resetForDecode(buffer_, bufferLen_)
        .subscribe(this, this, this);
    EXPECT_EQ(numFieldsSeen_, 4);
    EXPECT_EQ(numErrorsSeen_, 0);
    EXPECT_EQ(numCompletedsSeen_, 1);
}

class OtfMessageSetTest : public OtfMessageTest, public OtfMessageTestCBs
{
protected:
#define FIELD_SET_UINT8_VALUE 0x01
#define FIELD_SET_UINT16_VALUE 0x0200
#define FIELD_SET_UINT32_VALUE 0x00010000
#define FIELD_SET_UINT64_VALUE 0x0000000100000002L

    virtual void constructMessageIr(Ir &ir)
    {
        Ir::TokenByteOrder byteOrder = Ir::SBE_LITTLE_ENDIAN;
        std::string messageStr = std::string("MessageWithSets");
        std::string uint8FieldStr = std::string("SetUINT8Field");
        std::string uint16FieldStr = std::string("SetUINT16Field");
        std::string uint32FieldStr = std::string("SetUINT32Field");
        std::string uint64FieldStr = std::string("SetUINT64Field");
        std::string noChoicesFieldStr = std::string("SetNoChoicesField");

        ir.addToken(0, 23, Ir::BEGIN_MESSAGE, byteOrder, Ir::NONE, TEMPLATE_ID, messageStr);
        ir.addToken(0, 0, Ir::BEGIN_FIELD, byteOrder, Ir::NONE, FIELD_ID, uint8FieldStr);
        ir.addToken(0, 1, Ir::BEGIN_SET, byteOrder, Ir::UINT8, 0xFFFF, std::string("uint8"));
        ir.addToken(0, 0, Ir::CHOICE, byteOrder, Ir::UINT8, 0, std::string("uint8Choice0"));
        ir.addToken(0, 0, Ir::CHOICE, byteOrder, Ir::UINT8, 1, std::string("uint8Choice1"));
        ir.addToken(0, 0, Ir::END_SET, byteOrder, Ir::NONE, 0xFFFF, std::string("uint8"));
        ir.addToken(0, 0, Ir::END_FIELD, byteOrder, Ir::NONE, FIELD_ID, uint8FieldStr);
        ir.addToken(0, 0, Ir::BEGIN_FIELD, byteOrder, Ir::NONE, FIELD_ID + 1, uint16FieldStr);
        ir.addToken(1, 2, Ir::BEGIN_SET, byteOrder, Ir::UINT16, 0xFFFF, std::string("uint16"));
        ir.addToken(0, 0, Ir::CHOICE, byteOrder, Ir::UINT16, 9, std::string("uint16Choice9"));
        ir.addToken(0, 0, Ir::CHOICE, byteOrder, Ir::UINT16, 1, std::string("uint16Choice1"));
        ir.addToken(0, 0, Ir::END_SET, byteOrder, Ir::NONE, 0xFFFF, std::string("uint16"));
        ir.addToken(0, 0, Ir::END_FIELD, byteOrder, Ir::NONE, FIELD_ID + 1, uint16FieldStr);
        ir.addToken(0, 0, Ir::BEGIN_FIELD, byteOrder, Ir::NONE, FIELD_ID + 2, uint32FieldStr);
        ir.addToken(3, 4, Ir::BEGIN_SET, byteOrder, Ir::UINT32, 0xFFFF, std::string("uint32"));
        ir.addToken(0, 0, Ir::CHOICE, byteOrder, Ir::UINT32, 0, std::string("uint32Choice0"));
        ir.addToken(0, 0, Ir::CHOICE, byteOrder, Ir::UINT32, 16, std::string("uint32Choice16"));
        ir.addToken(0, 0, Ir::END_SET, byteOrder, Ir::NONE, 0xFFFF, std::string("uint32"));
        ir.addToken(0, 0, Ir::END_FIELD, byteOrder, Ir::NONE, FIELD_ID + 2, uint32FieldStr);
        ir.addToken(0, 0, Ir::BEGIN_FIELD, byteOrder, Ir::NONE, FIELD_ID + 3, uint64FieldStr);
        ir.addToken(7, 8, Ir::BEGIN_SET, byteOrder, Ir::UINT64, 0xFFFF, std::string("uint64"));
        ir.addToken(0, 0, Ir::CHOICE, byteOrder, Ir::UINT64, 32, std::string("uint64Choice32"));
        ir.addToken(0, 0, Ir::CHOICE, byteOrder, Ir::UINT64, 1, std::string("uint64Choice1"));
        ir.addToken(0, 0, Ir::END_SET, byteOrder, Ir::NONE, 0xFFFF, std::string("uint64"));
        ir.addToken(0, 0, Ir::END_FIELD, byteOrder, Ir::NONE, FIELD_ID + 3, uint64FieldStr);
        ir.addToken(0, 0, Ir::BEGIN_FIELD, byteOrder, Ir::NONE, FIELD_ID + 4, noChoicesFieldStr);
        ir.addToken(15, 8, Ir::BEGIN_SET, byteOrder, Ir::UINT64, 0xFFFF, std::string("uint64"));
        ir.addToken(0, 0, Ir::CHOICE, byteOrder, Ir::UINT64, 32, std::string("uint64Choice32"));
        ir.addToken(0, 0, Ir::CHOICE, byteOrder, Ir::UINT64, 1, std::string("uint64Choice1"));
        ir.addToken(0, 0, Ir::END_SET, byteOrder, Ir::NONE, 0xFFFF, std::string("uint64"));
        ir.addToken(0, 0, Ir::END_FIELD, byteOrder, Ir::NONE, FIELD_ID + 4, noChoicesFieldStr);
        ir.addToken(0, 23, Ir::END_MESSAGE, byteOrder, Ir::NONE, TEMPLATE_ID, messageStr);
    };

    virtual void constructMessage()
    {
        *((uint8_t *)(buffer_ + bufferLen_)) = FIELD_SET_UINT8_VALUE;
        bufferLen_ += sizeof(uint8_t);

        *((uint16_t *)(buffer_ + bufferLen_)) = FIELD_SET_UINT16_VALUE;
        bufferLen_ += sizeof(uint16_t);

        *((uint32_t *)(buffer_ + bufferLen_)) = FIELD_SET_UINT32_VALUE;
        bufferLen_ += sizeof(uint32_t);

        *((uint64_t *)(buffer_ + bufferLen_)) = FIELD_SET_UINT64_VALUE;
        bufferLen_ += sizeof(uint64_t);

        *((uint64_t *)(buffer_ + bufferLen_)) = 0;
        bufferLen_ += sizeof(uint64_t);
    };

    virtual int onNext(const Field &f)
    {
        OtfMessageTestCBs::onNext(f);

        if (numFieldsSeen_ == 2)
        {
            EXPECT_EQ(f.type(), Field::SET);
            EXPECT_EQ(f.numEncodings(), 1);
            EXPECT_EQ(f.fieldName(), "SetUINT8Field");
            EXPECT_EQ(f.primitiveType(), Ir::UINT8);
            EXPECT_EQ(f.getUInt(), FIELD_SET_UINT8_VALUE);
            EXPECT_EQ(f.choices().size(), 1);
            for (std::vector<std::string>::iterator it = ((std::vector<std::string>&)f.choices()).begin(); it != f.choices().end(); ++it)
            {
                EXPECT_TRUE(*it == "uint8Choice0");
            }
        }
        else if (numFieldsSeen_ == 3)
        {
            EXPECT_EQ(f.type(), Field::SET);
            EXPECT_EQ(f.numEncodings(), 1);
            EXPECT_EQ(f.fieldName(), "SetUINT16Field");
            EXPECT_EQ(f.primitiveType(), Ir::UINT16);
            EXPECT_EQ(f.getUInt(), FIELD_SET_UINT16_VALUE);
            EXPECT_EQ(f.choices().size(), 1);
            for (std::vector<std::string>::iterator it = ((std::vector<std::string>&)f.choices()).begin(); it != f.choices().end(); ++it)
            {
                EXPECT_TRUE(*it == "uint16Choice9");
            }
        }
        else if (numFieldsSeen_ == 4)
        {
            EXPECT_EQ(f.type(), Field::SET);
            EXPECT_EQ(f.numEncodings(), 1);
            EXPECT_EQ(f.fieldName(), "SetUINT32Field");
            EXPECT_EQ(f.primitiveType(), Ir::UINT32);
            EXPECT_EQ(f.getUInt(), FIELD_SET_UINT32_VALUE);
            EXPECT_EQ(f.choices().size(), 1);
            for (std::vector<std::string>::iterator it = ((std::vector<std::string>&)f.choices()).begin(); it != f.choices().end(); ++it)
            {
                EXPECT_TRUE(*it == "uint32Choice16");
            }
        }
        else if (numFieldsSeen_ == 5)
        {
            EXPECT_EQ(f.type(), Field::SET);
            EXPECT_EQ(f.numEncodings(), 1);
            EXPECT_EQ(f.fieldName(), "SetUINT64Field");
            EXPECT_EQ(f.primitiveType(), Ir::UINT64);
            EXPECT_EQ(f.getUInt(), FIELD_SET_UINT64_VALUE);
            EXPECT_EQ(f.choices().size(), 2);
            for (std::vector<std::string>::iterator it = ((std::vector<std::string>&)f.choices()).begin(); it != f.choices().end(); ++it)
            {
                EXPECT_TRUE(*it == "uint64Choice32" || *it == "uint64Choice1");
            }
        }
        else if (numFieldsSeen_ == 6)
        {
            EXPECT_EQ(f.type(), Field::SET);
            EXPECT_EQ(f.numEncodings(), 1);
            EXPECT_EQ(f.fieldName(), "SetNoChoicesField");
            EXPECT_EQ(f.primitiveType(), Ir::UINT64);
            EXPECT_EQ(f.getUInt(), 0);
            EXPECT_EQ(f.choices().size(), 0);
        }
        return 0;
    };
};

TEST_F(OtfMessageSetTest, shouldHandleSet)
{
    listener_.dispatchMessageByHeader(std::string("templateId"), messageHeaderIr_, this)
        .resetForDecode(buffer_, bufferLen_)
        .subscribe(this, this, this);
    EXPECT_EQ(numFieldsSeen_, 6);
    EXPECT_EQ(numErrorsSeen_, 0);
    EXPECT_EQ(numCompletedsSeen_, 1);
}

/*
 * TODO: variable length fields (with char and with uint8 types)
 * TODO: test reuse of listener
 * TODO: test offset values on fields
 * TODO: single repeating group
 * TODO: nested repeating group - MassQuote
 */