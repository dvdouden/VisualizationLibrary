/**************************************************************************************/
/*                                                                                    */
/*  Visualization Library                                                             */
/*  http://www.visualizationlibrary.org                                               */
/*                                                                                    */
/*  Copyright (c) 2005-2010, Michele Bosi                                             */
/*  All rights reserved.                                                              */
/*                                                                                    */
/*  Redistribution and use in source and binary forms, with or without modification,  */
/*  are permitted provided that the following conditions are met:                     */
/*                                                                                    */
/*  - Redistributions of source code must retain the above copyright notice, this     */
/*  list of conditions and the following disclaimer.                                  */
/*                                                                                    */
/*  - Redistributions in binary form must reproduce the above copyright notice, this  */
/*  list of conditions and the following disclaimer in the documentation and/or       */
/*  other materials provided with the distribution.                                   */
/*                                                                                    */
/*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND   */
/*  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED     */
/*  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE            */
/*  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR  */
/*  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES    */
/*  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;      */
/*  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON    */
/*  ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT           */
/*  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS     */
/*  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                      */
/*                                                                                    */
/**************************************************************************************/

#include <vlCore\SRF_Tools.hpp>

using namespace vl;

bool SRF_Tokenizer::getToken(SRF_Token& token)
{
  token.mType = SRF_Token::TOKEN_ERROR;
  token.mString.clear();

  if (mRawtextBlock)
    return getRawtextBlock(token);

  // read chars skipping spaces
  char ch1=0, ch2=0;
  do 
  {
    if (!readTextChar(ch1))
    {
      token.mType = SRF_Token::TOKEN_EOF;
      return true;
    }

    if (ch1 == '\n')
      ++mLineNumber;
    else
    // eat comments
    if (ch1 == '/')
    {
      if(readTextChar(ch2))
      {
        if (ch2 == '/') // single line comment
        {
          // eat everything till the end of the line
          for(ch1 = 0; readTextChar(ch1) && ch1 != '\n'; )
          {
            // eat everything
          }
          if (ch1 == '\n')
            ++mLineNumber;
        }
        else
        if (ch2 == '*') // multi line comment
        {
          // eat everything till the end of the line
          while(readTextChar(ch1))
          {
            if (ch1 == '*' && readTextChar(ch2) && ch2 == '/')
            {
              ch1 = '\n'; // pretend it's a space to stay in the loop
              break;
            }
            // eat everything
            if (ch1 == '\n')
              ++mLineNumber;
          }
        }
        else
        {
          Log::error( Say("Line %n : unexpected character '%c' after '/'.\n") << mLineNumber << ch2 );
          return false;
        }
      }
      else
      {
        Log::error( Say("Line %n : unexpected end of file in comment.\n") << mLineNumber);
        return false;
      }
      continue;
    }

  } while(ch1 == ' ' || ch1 == '\t' || ch1 == '\n');

  switch(ch1)
  {
  case '(':
    token.mType = SRF_Token::LeftRoundBracket;
    token.mString = "(";
    return true;
    
  case ')':
    token.mType = SRF_Token::RightRoundBracket;
    token.mString = ")";
    return true;

  case '[':
    token.mType = SRF_Token::LeftSquareBracket;
    token.mString = "[";
    return true;
    
  case ']':
    token.mType = SRF_Token::RightSquareBracket;
    token.mString = "]";
    return true;

  case '{':
    if(readTextChar(ch2) && ch2 == '<')
    {
      token.mType = SRF_Token::LeftFancyBracket;
      token.mString = "{<";
      mRawtextBlock = true;
      return true;
    }
    else
    {
      token.mType = SRF_Token::LeftCurlyBracket;
      token.mString = "{";
      if(!isEndOfFile())
        ungetToken(ch2);
    }
    return true;

  case '}':
    token.mType = SRF_Token::RightCurlyBracket;
    token.mString = "}";
    return true;

  case '>':
    if(readTextChar(ch2))
    {
      if(ch2 == '}')
      {
        token.mType = SRF_Token::RightFancyBracket;
        token.mString = ">}";
        return true;
      }
      else
      {
        Log::error( Say("Line %n : expected '}' instead of '%c' after '>'.\n") << mLineNumber << ch2 );
        return false;
      }
    }
    else
    {
        Log::error( Say("Line %n : unexpected end of file.\n") << mLineNumber );
        return false;
    }

  case '=':
    token.mType = SRF_Token::Equals; 
    token.mString = "=";
    return true;

  case '<':
    token.mString = "<";
    while(readTextChar(ch1) && ch1 != '>')
    {
      if (ch1 >= 'a' && ch1 <= 'z' || ch1 >= 'A' && ch1 <= 'Z' || ch1 >= '0' && ch1 <= '9' || ch1 == '_' || ch1 == ':')
        token.mString.push_back(ch1);
      else
      {
        Log::error( Say("Line %n : unexpected character '%c'.\n") << mLineNumber << ch1 );
        return false;
      }
    }
    token.mString.push_back('>');
    if (isEndOfFile())
    {
      Log::error( Say("Line %n : unexpected end of file while reading object header.\n") << mLineNumber );
      return false;
    }
    token.mType = SRF_Token::TagHeader;
    return true;

  case '#':
    token.mString = "#";
    while(readTextChar(ch1))
    {
      if (ch1 >= 'a' && ch1 <= 'z' || ch1 >= 'A' && ch1 <= 'Z' || ch1 >= '0' && ch1 <= '9' || ch1 == '_' )
        token.mString.push_back(ch1);
      else
      {
        ungetToken(ch1);
        break;
      }
    }
    if (token.mString == "#_")
    {
      Log::error( Say("Line %n : illegal id '#_' found.\n") << mLineNumber );
      return false;
    }
    token.mType = SRF_Token::UID;
    return true;

  case '"':
    while(readTextChar(ch1))
    {
      // end string
      if (ch1 == '"')
        break;
      else
      // return found before end of string
      if (ch1 == '\n')
      {
        Log::error( Say("Line %n : end of line found before end of string, did you forget a \"?.\n") << mLineNumber );
        return false;
      }
      else
      // escape sequences
      if (ch1 == '\\' && readTextChar(ch2))
      {
        if (ch2 == '"')
          ch1 = '"';
        else
        if (ch2 == '\\')
          ch1 = '\\';
        else
        if (ch2 == 'b')
          ch1 = '\b';
        else
        if (ch2 == 'f')
          ch1 = '\f';
        else
        if (ch2 == 'r')
          ch1 = '\r';
        else
        if (ch2 == 'n')
          ch1 = '\n';
        else
        if (ch2 == 't')
          ch1 = '\t';
        else
          ungetToken(ch2);
        token.mString.push_back(ch1);
      }
      else
      // accept everyhing else
        token.mString.push_back(ch1);
    }
    if (isEndOfFile())
    {
      Log::error( Say("Line %n : end of file found before end of string, did you forget a \"?.\n") << mLineNumber );
      return false;
    }
    else
    {
      token.mType = SRF_Token::String;
      return true;
    }

  default:
    // identifier
    if (ch1 >= 'a' && ch1 <= 'z' || ch1 >= 'A' && ch1 <= 'Z' || ch1 == '_' )
    {
      token.mString.push_back(ch1);
      while(readTextChar(ch1))
      {
        if (ch1 >= 'a' && ch1 <= 'z' || ch1 >= 'A' && ch1 <= 'Z' || ch1 >= '0' && ch1 <= '9' || ch1 == '_' )
          token.mString.push_back(ch1);
        else
        {
          ungetToken(ch1);
          break;
        }
      }
      if (token.mString == "_")
      {
        Log::error( Say("Line %n : unexpected character '_'.\n") << mLineNumber );
        return false;
      }
      else
      {
        // check if it's a boolean
        if (token.mString == "true" || token.mString == "false")
          token.mType = SRF_Token::Boolean;
        else
          token.mType = SRF_Token::Identifier;
        return true;
      }
    }
    else
    // Integer / Real
    //
    // ACCEPTED:
    // 123
    // +123.123E+10 -123.123e-10
    // +123
    // +.123
    // 0.123
    // 123.123
    //
    // REJECTED:
    // 01234
    // 01.234
    // 123.
    // 123.123E
    // 123.123e+
    if ( ch1 >= '0' && ch1 <= '9' || ch1 == '.' || ch1 == '+' || ch1 == '-' )
    {
      token.mType = SRF_Token::TOKEN_ERROR;
      token.mString.push_back(ch1);

      enum { sZERO, sPLUS_MINUS, sINT, sFRAC, sPOINT, sE, sPLUS_MINUS_EXP, sEXP } state = sINT;

      if ( ch1 >= '1' && ch1 <= '9' )
        state = sINT;
      else
      if (ch1 == '0')
        state = sZERO;
      else
      if (ch1 == '.')
        state = sPOINT;
      else
      if (ch1 == '+' || ch1 == '-')
        state = sPLUS_MINUS;

      while(readTextChar(ch1))
      {
        switch(state)
        {
        // if starting with 0 must be 0.0-9
        case sZERO:
          if (ch1 == '.')
          {
            token.mString.push_back(ch1);
            state = sPOINT;
          }
          else
          {
            token.mType = SRF_Token::Integer;
            ungetToken(ch1);
            return true;
          }
          break;

        case sPLUS_MINUS:
          if (ch1 == '0')
          {
            token.mString.push_back(ch1);
            state = sZERO;
          }
          else
          if (ch1 >= '1' && ch1 <= '9')
          {
            token.mString.push_back(ch1);
            state = sINT;
          }
          else
          if (ch1 == '.')
          {
            token.mString.push_back(ch1);
            state = sPOINT;
          }
          else
          {
            Log::error( Say("Line %n :unexpected character '%c'.\n") << mLineNumber << ch1 );
            return false;
          }
          break;

        case sINT:
          if (ch1 >= '0' && ch1 <= '9')
            token.mString.push_back(ch1);
          else
          if (ch1 == '.')
          {
            token.mString.push_back(ch1);
            state = sPOINT;
          }
          else
          {
            token.mType = SRF_Token::Integer;
            ungetToken(ch1);
            return true;
          }
          break;

        case sPOINT:
          if (ch1 >= '0' && ch1 <= '9')
          {
            token.mString.push_back(ch1);
            state = sFRAC;
          }
          else
          {
            Log::error( Say("Line %n :unexpected character '%c'.\n") << mLineNumber << ch1 );
            return false;
          }
          break;

        case sFRAC:
          if (ch1 >= '0' && ch1 <= '9')
            token.mString.push_back(ch1);
          else
          if (ch1 == 'E' || ch1 == 'e')
          {
            token.mString.push_back(ch1);
            state = sE;
          }
          else
          {
            token.mType = SRF_Token::Real;
            ungetToken(ch1);
            return true;
          }
          break;

        case sE:
          if (ch1 == '+' || ch1 == '-')
          {
            token.mString.push_back(ch1);
            state = sPLUS_MINUS_EXP;
          }
          else
          {
            Log::error( Say("Line %n :unexpected character '%c'.\n") << mLineNumber << ch1 );
            return false;
          }
          break;

        case sPLUS_MINUS_EXP:
          if (ch1 >= '0' && ch1 <= '9')
          {
            token.mString.push_back(ch1);
            state = sEXP;
          }
          else
          {
            Log::error( Say("Line %n :unexpected character '%c'.\n") << mLineNumber << ch1 );
            return false;
          }
          break;

        case sEXP:
          if (ch1 >= '0' && ch1 <= '9')
            token.mString.push_back(ch1);
          else
          {
            token.mType = SRF_Token::Real;
            ungetToken(ch1);
            return true;
          }
          break;
        }
      }
      // reached TOKEN_EOF in the middle of the parsing so we check where we were, note that it cannot be a Integer or a Real
      if (state == sINT)
      {
        token.mType = SRF_Token::Integer;
        return true;
      }
      else
      if (state == sFRAC || state == sEXP)
      {
        token.mType = SRF_Token::Real;
        return true;
      }
      else
        return false;
    }
    else
    {
      Log::error( Say("Line %n : unexpected character '%c'.\n") << mLineNumber << ch1 );
      return false;
    }
  }
}
//-----------------------------------------------------------------------------
bool SRF_Tokenizer::getRawtextBlock(SRF_Token& token)
{
  mRawtextBlock = false;

  token.mString.clear();
  token.mType = SRF_Token::TOKEN_ERROR;

  char ch =0;
  while(readTextChar(ch))
  {
    if (ch == '\n')
      ++mLineNumber;

    if (ch == '>')
    {
      // check for rawtext block end >}
      char ch2 = 0;
      if (readTextChar(ch2))
      {
        if(ch2 == '}')
        {
          // check if it was escaped
          if (!token.mString.empty() && token.mString[ token.mString.size() - 1 ] == '\\')
          {
            token.mString.resize( token.mString.size() - 1 );
            token.mString += ">}";
            continue;
          }
          else
          {
            token.mType = SRF_Token::RawtextBlock;
            ungetToken('}');
            ungetToken('>');
            return true;
          }
        }
        else
          ungetToken(ch2);
      }
    }
    
    token.mString.push_back(ch);
  }

  return false;
}
//-----------------------------------------------------------------------------
void SRF_Value::release()
{
  switch(mType)
  {
  case Structure: 
    if (mUnion.mStructure)
      mUnion.mStructure->decReference(); 
    break;

  case List:
    if (mUnion.mList)
      mUnion.mList->decReference(); 
    break;

  case RawtextBlock:
    if (mUnion.mRawtextBlock)
      mUnion.mRawtextBlock->decReference(); 
    break;

  case ArrayString:
  case ArrayUID:
  case ArrayIdentifier:
  case ArrayInteger:
  case ArrayReal:
    if (mUnion.mArray)
      mUnion.mArray->decReference(); 
    break;

  case String:
  case UID:
  case Identifier:
    VL_CHECK(mUnion.mString)
    free((void*)mUnion.mString);
    mUnion.mString = NULL; break;
    break;

  default:
    break;
  }

  mType = Integer;
  mUnion.mInteger = 0;
}
//-----------------------------------------------------------------------------
SRF_Value& SRF_Value::operator=(const SRF_Value& other)
{
  mLineNumber = other.mLineNumber;

  // must be done first
  switch(other.mType)
  {
  case Structure:
    if (other.mUnion.mStructure)
      other.mUnion.mStructure->incReference(); 
    break;

  case List:
    if (other.mUnion.mList)
      other.mUnion.mList->incReference(); 
    break;

  case RawtextBlock:
    if (other.mUnion.mRawtextBlock)
      other.mUnion.mRawtextBlock->incReference(); 
    break;

  case ArrayString:
  case ArrayUID:
  case ArrayIdentifier:
  case ArrayInteger:
  case ArrayReal:
    if (other.mUnion.mArray)
      other.mUnion.mArray->incReference(); 
    break;

  default:
    break;
  }

  // must be done after
  release();

  mUnion = other.mUnion;
  mType = other.mType;

  // make local copy of the string
  if (mType == String || mType == Identifier || mType == UID)
    mUnion.mString = _strdup(mUnion.mString);

  return *this;
}
//-----------------------------------------------------------------------------
SRF_Structure* SRF_Value::setStructure(SRF_Structure* obj)
{
  release();
  mType = Structure;
  mUnion.mStructure = obj;
  if (mUnion.mStructure)
    mUnion.mStructure->incReference();
  return obj;
}
//-----------------------------------------------------------------------------
SRF_List* SRF_Value::setList(SRF_List* list)
{
  VL_CHECK(list);

  release();
  mType = List;
  mUnion.mList = list;
  if (mUnion.mList)
    mUnion.mList->incReference();
  return list;
}
//-----------------------------------------------------------------------------
SRF_RawtextBlock* SRF_Value::setRawtextBlock(SRF_RawtextBlock* fblock)
{
  VL_CHECK(fblock);

  release();
  mType = RawtextBlock;
  mUnion.mRawtextBlock = fblock;
  if (mUnion.mRawtextBlock)
    mUnion.mRawtextBlock->incReference();
  return fblock;
}
//-----------------------------------------------------------------------------
SRF_ArrayInteger* SRF_Value::setArrayInteger(SRF_ArrayInteger* arr)
{
  VL_CHECK(arr);
  release();
  mType = ArrayInteger;
  mUnion.mArray = arr;
  if (mUnion.mArray)
    mUnion.mArray->incReference();
  return arr;
}
//-----------------------------------------------------------------------------
SRF_ArrayReal* SRF_Value::setArrayReal(SRF_ArrayReal* arr)
{
  VL_CHECK(arr);
  release();
  mType = ArrayReal;
  mUnion.mArray = arr;
  if (mUnion.mArray)
    mUnion.mArray->incReference();
  return arr;
}
//-----------------------------------------------------------------------------
SRF_ArrayIdentifier* SRF_Value::setArrayIdentifier(SRF_ArrayIdentifier* arr)
{
  VL_CHECK(arr);
  release();
  mType = ArrayIdentifier;
  mUnion.mArray = arr;
  if (mUnion.mArray)
    mUnion.mArray->incReference();
  return arr;
}
//-----------------------------------------------------------------------------
SRF_ArrayString* SRF_Value::setArrayString(SRF_ArrayString* arr)
{
  VL_CHECK(arr);
  release();
  mType = ArrayString;
  mUnion.mArray = arr;
  if (mUnion.mArray)
    mUnion.mArray->incReference();
  return arr;
}
//-----------------------------------------------------------------------------
SRF_ArrayUID* SRF_Value::setArrayUID(SRF_ArrayUID* arr)
{
  VL_CHECK(arr);
  release();
  mType = ArrayUID;
  mUnion.mArray = arr;
  if (mUnion.mArray)
    mUnion.mArray->incReference();
  return arr;
}
//-----------------------------------------------------------------------------
SRF_Array* SRF_Value::setArray(SRF_Array* arr)
{
  if (arr->classType() == SRF_ArrayInteger::Type())
    return setArrayInteger(arr->as<SRF_ArrayInteger>());
  else
  if (arr->classType() == SRF_ArrayReal::Type())
    return setArrayReal(arr->as<SRF_ArrayReal>());
  else
  if (arr->classType() == SRF_ArrayIdentifier::Type())
    return setArrayIdentifier(arr->as<SRF_ArrayIdentifier>());
  else
  if (arr->classType() == SRF_ArrayString::Type())
    return setArrayString(arr->as<SRF_ArrayString>());
  else
  if (arr->classType() == SRF_ArrayUID::Type())
    return setArrayUID(arr->as<SRF_ArrayUID>());
  else
  {
    VL_TRAP();
    return NULL;
  }
}
//-----------------------------------------------------------------------------