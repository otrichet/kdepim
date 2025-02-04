/*
    kpgpbase2.cpp

    Copyright (C) 2001,2002 the KPGP authors
    See file AUTHORS.kpgp for details

    This file is part of KPGP, the KDE PGP/GnuPG support library.

    KPGP is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software Foundation,
    Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "kpgpbase.h"
#include "kpgp.h"

#include <string.h> /* strncmp */
#include <assert.h>

#include <QDateTime>
//Added by qt3to4:
#include <QByteArray>

#include <klocale.h>
#include <kshell.h>
#include <kdebug.h>

#include <algorithm>

#define PGP2 "pgp"

namespace Kpgp {

Base2::Base2()
  : Base()
{
}


Base2::~Base2()
{
}


int
Base2::encrypt( Block& block, const KeyIDList& recipients )
{
  return encsign( block, recipients, 0 );
}


int
Base2::clearsign( Block& block, const char *passphrase )
{
  return encsign( block, KeyIDList(), passphrase );
}


int
Base2::encsign( Block& block, const KeyIDList& recipients,
                const char *passphrase )
{
  QByteArray cmd;
  int exitStatus = 0;

  if(!recipients.isEmpty() && passphrase != 0)
    cmd = PGP2 " +batchmode +language=en +verbose=1 -seat";
  else if(!recipients.isEmpty())
    cmd = PGP2 " +batchmode +language=en +verbose=1 -eat";
  else if(passphrase != 0)
    cmd = PGP2 " +batchmode +language=en +verbose=1 -sat";
  else
  {
    kDebug( 5326 ) <<"kpgpbase: Neither recipients nor passphrase specified.";
    return OK;
  }

  if(passphrase != 0)
    cmd += addUserId();

  if(!recipients.isEmpty()) {
    if(Module::getKpgp()->encryptToSelf())
    {
      cmd += " 0x";
      cmd += Module::getKpgp()->user();
    }

    for( KeyIDList::ConstIterator it = recipients.begin();
         it != recipients.end(); ++it ) {
      cmd += " 0x";
      cmd += (*it);
    }
  }
  cmd += " -f";

  clear();
  input = block.text();
  exitStatus = run(cmd.data(), passphrase);
  if( !output.isEmpty() )
    block.setProcessedText( output );
  block.setError( error );

  if(exitStatus != 0)
    status = ERROR;

#if 0
  // #### FIXME: As we check the keys ourselves the following problems
  //             shouldn't occur. Therefore I don't handle them for now.
  //             IK 01/2002
  if(!recipients.isEmpty())
  {
    int index = 0;
    bool bad = false;
    unsigned int num = 0;
    QByteArray badkeys = "";
    if (error.contains("Cannot find the public key") )
    {
      index = 0;
      num = 0;
      while((index = error.indexOf("Cannot find the public key",index))
	    != -1)
      {
        bad = true;
        index = error.indexOf('\'',index);
        int index2 = error.indexOf('\'',index+1);
        if (num++)
          badkeys += ", ";
        badkeys += error.mid(index, index2-index+1);
      }
      if(bad)
      {
        badkeys.trimmed();
        if(num == recipients.count())
	  errMsg = i18n("Could not find public keys matching the userid(s)\n"
                        "%1;\n"
                        "the message is not encrypted.",
                         badkeys.data() );
        else
          errMsg = i18n("Could not find public keys matching the userid(s)\n"
                        "%1;\n"
                        "these persons will not be able to read the message.",
                         badkeys.data() );
        status |= MISSINGKEY;
        status |= ERROR;
      }
    }
    if (error.contains("skipping userid") )
    {
      index = 0;
      num = 0;
      while((index = error.indexOf("skipping userid",index))
	    != -1)
      {
        bad = true;
        int index2 = error.indexOf('\n',index+16);
        if (num++)
          badkeys += ", ";
        badkeys += error.mid(index+16, index2-index-16);
        index = index2;
      }
      if(bad)
      {
        badkeys.trimmed();
        if(num == recipients.count())
	  errMsg = i18n("Public keys not certified with trusted signature "
                        "for userid(s)\n"
                        "%1.\n"
                        "The message is not encrypted.",
                         badkeys.data() );
        else
	  errMsg = i18n("Public keys not certified with trusted signature "
                        "for userid(s)\n"
                        "%1;\n"
                        "these persons will not be able to read the message.",
                         badkeys.data() );
        status |= BADKEYS;
        status |= ERROR;
        return status;
      }
    }
  }
#endif
  if(passphrase != 0)
  {
    if(error.contains("Pass phrase is good") )
    {
      //kDebug( 5326 ) <<"Base: Good Passphrase!";
      status |= SIGNED;
    }
    if( error.contains("Bad pass phrase") )
    {
      errMsg = i18n("Bad passphrase; could not sign.");
      status |= BADPHRASE;
      status |= ERR_SIGNING;
      status |= ERROR;
    }
  }
  if (error.contains("Signature error") )
  {
    errMsg = i18n("Signing failed: please check your PGP User Identity, "
                  "the PGP setup, and the key rings.");
    status |= NO_SEC_KEY;
    status |= ERR_SIGNING;
    status |= ERROR;
  }
  if (error.contains("Encryption error") )
  {
    errMsg = i18n("Encryption failed: please check your PGP setup "
                  "and the key rings.");
    status |= NO_SEC_KEY;
    status |= BADKEYS;
    status |= ERROR;
  }

  //kDebug( 5326 ) <<"status =" << status;
  block.setStatus( status );
  return status;
}


int
Base2::decrypt( Block& block, const char *passphrase )
{
  int index, index2;
  int exitStatus = 0;

  clear();
  input = block.text();
  exitStatus = run(PGP2 " +batchmode +language=en -f", passphrase);
  if( !output.isEmpty() )
    block.setProcessedText( output );
  block.setError( error );

  // pgp2.6 has sometimes problems with the ascii armor pgp5.0 produces
  // this hack can solve parts of the problem
  if(error.contains("ASCII armor corrupted.") )
  {
    kDebug( 5326 ) <<"removing ASCII armor header";
    int index1 = input.indexOf("-----BEGIN PGP SIGNED MESSAGE-----");
    if(index1 != -1)
      index1 = input.indexOf("-----BEGIN PGP SIGNATURE-----", index1);
    else
      index1 = input.indexOf("-----BEGIN PGP MESSAGE-----");
    index1 = input.indexOf('\n', index1);
    index2 = input.indexOf("\n\n", index1);
    input.remove(index1, index2 - index1);
    exitStatus = run(PGP2 " +batchmode +language=en -f", passphrase);
    if( !output.isEmpty() )
      block.setProcessedText( output );
    block.setError( error );
  }

  if(exitStatus == -1) {
    errMsg = i18n("error running PGP");
    status = ERROR;
    block.setStatus( status );
    return status;
  }

  /* Example No.1 (PGP 2.6.3in):
   * File is encrypted.  Secret key is required to read it.
   * Key for user ID: Test Key (only for testing) <testkey@ingo-kloecker.de>
   * 1024-bit key, key ID E2D074D3, created 2001/09/09
   *
   * Error:  Bad pass phrase.
   *
   * This message can only be read by:
   *   Test key without secret key (for testing only) <nosectestkey@ingo-kloecker.de>
   *   Test Key (only for testing) <testkey@ingo-kloecker.de>
   *
   * You do not have the secret key needed to decrypt this file.
   */
  /* Example No.2 (PGP 2.6.3in):
   * File is encrypted.  Secret key is required to read it.
   * This message can only be read by:
   *   Test key without secret key (for testing only) <nosectestkey@ingo-kloecker.de>
   *
   * You do not have the secret key needed to decrypt this file.
   */
  if(error.contains("File is encrypted.") )
  {
    //kDebug( 5326 ) <<"kpgpbase: message is encrypted";
    status |= ENCRYPTED;
    if((index = error.indexOf("Key for user ID:")) != -1 )
    {
      // Find out the key for which the phrase is needed
      index  += 17;
      index2 = error.indexOf('\n', index);
      block.setRequiredUserId( error.mid(index, index2 - index) );
      //kDebug( 5326 ) <<"Base: key needed is \"" << block.requiredUserId() <<"\"!";

      if((passphrase != 0) && (error.contains("Bad pass phrase") ))
      {
        errMsg = i18n("Bad passphrase; could not decrypt.");
        kDebug( 5326 ) <<"Base: passphrase is bad";
        status |= BADPHRASE;
        status |= ERROR;
      }
    }
    else
    {
      // no secret key fitting this message
      status |= NO_SEC_KEY;
      status |= ERROR;
      errMsg = i18n("You do not have the secret key needed to decrypt this message.");
      kDebug( 5326 ) <<"Base: no secret key for this message";
    }
    // check for persons
#if 0
    // ##### FIXME: This information is anyway currently not used
    //       I'll change it to always determine the recipients.
    index = error.indexOf("can only be read by:");
    if(index != -1)
    {
      index = error.indexOf('\n',index);
      int end = error.indexOf("\n\n",index);

      mRecipients.clear();
      while( (index2 = error.indexOf('\n',index+1)) <= end )
      {
	QByteArray item = error.mid(index+1,index2-index-1);
	item.trimmed();
	mRecipients.append(item);
	index = index2;
      }
    }
#endif
  }

  // handle signed message

  // Examples (made with PGP 2.6.3in)
  /* Example No. 1 (signed with unknown key):
   * File has signature.  Public key is required to check signature.
   *
   * Key matching expected Key ID 12345678 not found in file '/home/user/.pgp/pubring.pgp'.
   *
   * WARNING: Can't find the right public key-- can't check signature integrity.
   */
  /* Example No. 2 (bad signature):
   * File has signature.  Public key is required to check signature.
   * ..
   * WARNING: Bad signature, doesn't match file contents!
   *
   * Bad signature from user "Joe User <joe@foo.bar>".
   * Signature made 2001/09/09 16:01 GMT using 1024-bit key, key ID 12345678
   */
  /* Example No. 3.1 (good signature with untrusted key):
   * File has signature.  Public key is required to check signature.
   * .
   * Good signature from user "Joe User <joe@foo.bar>".
   * Signature made 2001/09/09 16:01 GMT using 1024-bit key, key ID 12345678
   *
   * WARNING:  Because this public key is not certified with a trusted
   * signature, it is not known with high confidence that this public key
   * actually belongs to: "Joe User <joe@foo.bar>".
   */
  /* Example No. 3.2 (good signature with untrusted key):
   * File has signature.  Public key is required to check signature.
   * .
   * Good signature from user "Joe User <joe@foo.bar>".
   * Signature made 2001/09/09 16:01 GMT using 1024-bit key, key ID 12345678
   *
   * WARNING:  Because this public key is not certified with enough trusted
   * signatures, it is not known with high confidence that this public key
   * actually belongs to: "Joe User <joe@foo.bar>".
   */
  /* Example No. 4 (good signature with revoked key):
   * File has signature.  Public key is required to check signature.
   * .
   * Good signature from user "Joe User <joe@foo.bar>".
   * Signature made 2001/09/09 16:01 GMT using 1024-bit key, key ID 12345678
   *
   *
   * Key for user ID: Joe User <joe@foo.bar>
   * 1024-bit key, key ID 12345678, created 2001/09/09
   * Key has been revoked.
   *
   * WARNING:  This key has been revoked by its owner,
   * possibly because the secret key was compromised.
   * This could mean that this signature is a forgery.
   */
  /* Example No. 5 (good signature with trusted key):
   * File has signature.  Public key is required to check signature.
   * .
   * Good signature from user "Joe User <joe@foo.bar>".
   * Signature made 2001/09/09 16:01 GMT using 1024-bit key, key ID 12345678
   */

  if((index = error.indexOf("File has signature")) != -1 )
  {
    // move index to start of next line
    index = error.indexOf('\n', index+18) + 1;
    //kDebug( 5326 ) <<"Base: message is signed";
    status |= SIGNED;
    // get signature date and signature key ID
    if ((index2 = error.indexOf("Signature made", index)) != -1 ) {
      index2 += 15;
      int index3 = error.indexOf("using", index2);
      block.setSignatureDate( error.mid(index2, index3-index2-1) );
      kDebug( 5326 ) <<"Message was signed on '" << block.signatureDate() <<"'";
      index3 = error.indexOf("key ID ", index3) + 7;
      block.setSignatureKeyId( error.mid(index3,8) );
      kDebug( 5326 ) <<"Message was signed with key '" << block.signatureKeyId() <<"'";
    }
    else {
      // if pgp can't find the keyring it unfortunately doesn't print
      // the signature date and key ID
      block.setSignatureDate( "" );
      block.setSignatureKeyId( "" );
    }

    if( ( index2 = error.indexOf("Key matching expected", index) ) != -1 )
    {
      status |= UNKNOWN_SIG;
      status |= GOODSIG;
      int index3 = error.indexOf("Key ID ", index2) + 7;
      block.setSignatureKeyId( error.mid(index3,8) );
      block.setSignatureUserId( QString() );
    }
    else if( (index2 = error.indexOf("Good signature from", index)) != -1 )
    {
      status |= GOODSIG;
      // get signer
      index = error.indexOf('"',index2+19);
      index2 = error.indexOf('"', index+1);
      block.setSignatureUserId( error.mid(index+1, index2-index-1) );
    }
    else if( (index2 = error.indexOf("Bad signature from", index)) != -1 )
    {
      status |= ERROR;
      // get signer
      index = error.indexOf('"',index2+19);
      index2 = error.indexOf('"', index+1);
      block.setSignatureUserId( error.mid(index+1, index2-index-1) );
    }
    else if( error.indexOf("Keyring file", index) != -1 )
    {
      // #### fix this hack
      status |= UNKNOWN_SIG;
      status |= GOODSIG; // this is a hack...
      // determine file name of missing keyring file
      index = error.indexOf('\'', index) + 1;
      index2 = error.indexOf('\'', index);
      block.setSignatureUserId( i18n("The keyring file %1 does not exist.\n"
      "Please check your PGP setup.", QString::fromLatin1( error.mid(index, index2-index)) ) );
    }
    else
    {
      status |= ERROR;
      block.setSignatureUserId( i18n("Unknown error") );
    }
  }
  //kDebug( 5326 ) <<"status =" << status;
  block.setStatus( status );
  return status;
}


Key*
Base2::readPublicKey( const KeyID& keyID,
                      const bool readTrust /* = false */,
                      Key* key /* = 0 */ )
{
  int exitStatus = 0;

  status = 0;
  exitStatus = run( PGP2 " +batchmode +language=en +verbose=0 -kvc -f 0x" +
                    keyID, 0, true );

  if(exitStatus != 0) {
    status = ERROR;
    return 0;
  }

  key = parsePublicKeyData( output, key );

  if( key == 0 )
  {
    return 0;
  }

  if( readTrust )
  {
    exitStatus = run( PGP2 " +batchmode +language=en +verbose=0 -kc -f",
                      0, true );

    if(exitStatus != 0) {
      status = ERROR;
      return 0;
    }

    parseTrustDataForKey( key, error );
  }

  return key;
}


KeyList
Base2::publicKeys( const QStringList & patterns )
{
  return doGetPublicKeys( PGP2 " +batchmode +language=en +verbose=0 -kvc -f",
                          patterns );
}

KeyList
Base2::doGetPublicKeys( const QByteArray & cmd, const QStringList & patterns )
{
  int exitStatus = 0;
  KeyList publicKeys;

  status = 0;
  if ( patterns.isEmpty() ) {
    exitStatus = run( cmd, 0, true );

    if ( exitStatus != 0 ) {
      status = ERROR;
      return KeyList();
    }

    // now we need to parse the output for public keys
    publicKeys = parseKeyList( output, false );
  }
  else {
    typedef QMap<QByteArray, Key*> KeyMap;
    KeyMap map;

    for ( QStringList::ConstIterator it = patterns.begin();
          it != patterns.end(); ++it ) {
      exitStatus = run( cmd + ' ' + KShell::quoteArg( *it ).toLocal8Bit(),
                        0, true );

      if ( exitStatus != 0 ) {
        status = ERROR;
        return KeyList();
      }

      // now we need to parse the output for public keys
      publicKeys = parseKeyList( output, false );

      // put all new keys into a map, remove duplicates
      while ( !publicKeys.isEmpty() ) {
        Key * key = publicKeys.takeFirst();
        if ( !map.contains( key->primaryFingerprint() ) )
          map.insert( key->primaryFingerprint(), key );
        else
          delete key;
      }
    }
    // build list from the map
    for ( KeyMap::ConstIterator it = map.constBegin(); it != map.constEnd(); ++it ) {
      publicKeys.append( it.value() );
    }
  }

  // sort the list of public keys
  std::sort( publicKeys.begin(), publicKeys.end(), KeyCompare );

  return publicKeys;
}

KeyList
Base2::secretKeys( const QStringList & patterns )
{
  return publicKeys( patterns );
}


int
Base2::signKey(const KeyID& keyID, const char *passphrase)
{
  QByteArray cmd;
  int exitStatus = 0;

  cmd = PGP2 " +batchmode +language=en -ks -f ";
  cmd += addUserId();
  cmd += " 0x" + keyID;

  status = 0;
  exitStatus = run(cmd.data(),passphrase);

  if (exitStatus != 0)
    status = ERROR;

  return status;
}


QByteArray Base2::getAsciiPublicKey(const KeyID& keyID)
{
  int exitStatus = 0;

  if (keyID.isEmpty())
    return QByteArray();

  status = 0;
  exitStatus = run( PGP2 " +batchmode +force +language=en -kxaf 0x" + keyID,
                    0, true );

  if(exitStatus != 0) {
    status = ERROR;
    return QByteArray();
  }

  return output;
}


Key*
Base2::parsePublicKeyData( const QByteArray& output, Key* key /* = 0 */ )
{
  Subkey *subkey = 0;
  int index;

  // search start of key data
  if( !strncmp( output.data(), "pub", 3 ) ||
      !strncmp( output.data(), "sec", 3 ) )
    index = 0;
  else
  {
    /*
    if( secretKeys )
      index = output.indexOf( "\nsec" );
    else
    */
      index = output.indexOf( "\npub" );
    if( index == -1 )
      return 0;
    else
      index++;
  }

  while( true )
  {
    int index2;

    // search the end of the current line
    if( ( index2 = output.indexOf( '\n', index ) ) == -1 )
      break;

    if( !strncmp( output.data() + index, "pub", 3 ) ||
        !strncmp( output.data() + index, "sec", 3 ) )
    { // line contains primary key data
      // Example 1 (nothing special):
      // pub  1024/E2D074D3 2001/09/09 Test Key <testkey@xyz>
      // Example 2 (disabled key):
      // pub- 1024/8CCB2C1B 2001/11/04 Disabled Test Key <disabled@xyz>
      // Example 3 (expired key):
      // pub> 1024/7B94827D 2001/09/09 Expired Test Key <expired@xyz> (EXPIRE:2001-09-10)
      // Example 4 (revoked key):
      // pub  1024/956721F9 2001/09/09 *** KEY REVOKED ***

      int pos, pos2;

      if( key == 0 )
        key = new Key();
      else
        key->clear();
      /*key->setSecret( secretKeys );*/
      // set default key capabilities
      key->setCanEncrypt( true );
      key->setCanSign( true );
      key->setCanCertify( true );

      /*subkey = new Subkey( "", secretKeys );*/
      subkey = new Subkey( "", false );
      key->addSubkey( subkey );
      // set default key capabilities
      subkey->setCanEncrypt( true );
      subkey->setCanSign( true );
      subkey->setCanCertify( true );
      // expiration date defaults to never
      subkey->setExpirationDate( -1 );

      // Key Flags
      switch( output[index+3] )
      {
      case ' ': // nothing special
        break;
      case '-': // disabled key
        subkey->setDisabled( true );
        key->setDisabled( true );
        break;
      case '>': // expired key
        subkey->setExpired( true );
        key->setExpired( true );
        break;
      default:
        kDebug( 5326 ) <<"Unknown key flag.";
      }

      // Key Length
      pos = index + 4;
      while( output[pos] == ' ' )
        pos++;
      pos2 = output.indexOf( '/', pos );
      subkey->setKeyLength( output.mid( pos, pos2-pos ).toUInt() );

      // Key ID
      pos = pos2 + 1;
      pos2 = output.indexOf( ' ', pos );
      subkey->setKeyID( output.mid( pos, pos2-pos ) );

      // Creation Date
      pos = pos2 + 1;
      while( output[pos] == ' ' )
        pos++;
      pos2 = output.indexOf( ' ', pos );
      int year = output.mid( pos, 4 ).toInt();
      int month = output.mid( pos+5, 2 ).toInt();
      int day = output.mid( pos+8, 2 ).toInt();
      QDateTime dt( QDate( year, month, day ), QTime( 00, 00 ) );
      QDateTime epoch( QDate( 1970, 01, 01 ), QTime( 00, 00 ) );
      // The calculated creation date isn't exactly correct because QDateTime
      // doesn't know anything about timezones and always assumes local time
      // although epoch is of course UTC. But as PGP 2 anyway doesn't print
      // the time this doesn't matter too much.
      subkey->setCreationDate( epoch.secsTo( dt ) );

      // User ID
      pos = pos2 + 1;
      while( output[pos] == ' ' )
        pos++;
      QByteArray uid = output.mid( pos, index2-pos );
      if( uid != "*** KEY REVOKED ***" )
        key->addUserID( uid );
      else
      {
        subkey->setRevoked( true );
        key->setRevoked( true );
      }
    }
    else if( output[index] == ' ' )
    { // line contains additional key data

      if( key == 0 )
        break;
      assert( subkey != 0 );

      int pos = index + 1;
      while( output[pos] == ' ' )
        pos++;

      if( !strncmp( output.data() + pos, "Key fingerprint = ", 18 ) )
      { // line contains a fingerprint
        // Example:
        //             Key fingerprint = 47 30 7C 76 05 BF 5E FB  72 41 00 F2 7D 0B D0 49

        QByteArray fingerprint = output.mid( pos, index2-pos );
        // remove white space from the fingerprint
	for ( int idx = 0 ; (idx = fingerprint.indexOf(' ', idx)) != -1 ; )
	  fingerprint.replace( idx, 1, "" );

        subkey->setFingerprint( fingerprint );
      }
      else if( !strncmp( output.data() + pos, "Expire: ", 8 ) ||
               !strncmp( output.data() + pos, "no expire ", 10 ) )
      { // line contains additional key properties
        // Examples:
        //            Expire: 2001/09/10
        //                     no expire ENCRyption only
        //                     no expire SIGNature only

        if( output[pos] == 'E' )
        {
          // Expiration Date
          pos += 8;
          int year = output.mid( pos, 4 ).toInt();
          int month = output.mid( pos+5, 2 ).toInt();
          int day = output.mid( pos+8, 2 ).toInt();
          QDateTime dt( QDate( year, month, day ), QTime( 00, 00 ) );
          QDateTime epoch( QDate( 1970, 01, 01 ), QTime( 00, 00 ) );
          // Here the same comments as for the creation date are valid.
          subkey->setExpirationDate( epoch.secsTo( dt ) );
          pos += 11; // note that there is always a blank after the expire date
        }
        else
          pos += 10;

        // optional key capabilities (sign/encrypt only)
        if( pos != index2 )
        {
          if( !strncmp( output.data() + pos, "SIGNature only", 14 ) )
          {
            subkey->setCanEncrypt( false );
            key->setCanEncrypt( false );
          }
          else if( !strncmp( output.data() + pos, "ENCRyption only", 15 ) )
          {
            subkey->setCanSign( false );
            key->setCanSign( false );
            subkey->setCanCertify( false );
            key->setCanCertify( false );
          }
        }
      }
      else
      { // line contains an additional user id
        // Example:
        //                               Test key (2nd user ID) <abc@xyz>

        key->addUserID( output.mid( pos, index2-pos ) );
      }
    }
    index = index2 + 1;
  }

  //kDebug( 5326 ) <<"finished parsing key data";

  return key;
}


void
Base2::parseTrustDataForKey( Key* key, const QByteArray& str )
{
  if( ( key == 0 ) || str.isEmpty() )
    return;

  QByteArray keyID = key->primaryKeyID();
  UserIDList userIDs = key->userIDs();

  // search the trust data belonging to this key
  int index = str.indexOf( '\n' ) + 1;
  while( ( index > 0 ) &&
         ( strncmp( str.data() + index+2, keyID.data(), 8 ) != 0 ) )
    index = str.indexOf( '\n', index ) + 1;

  if( index == 0 )
    return;

  bool ultimateTrust = false;
  if( !strncmp( str.data() + index+11, "ultimate", 8 ) )
    ultimateTrust = true;

  bool firstLine = true;

  while( true )
  { // loop over all trust information about this key
    int index2;

    // search the end of the current line
    if( ( index2 = str.indexOf( '\n', index ) ) == -1 )
      break;

    // check if trust info for the next key starts
    if( !firstLine && ( str[index+2] != ' ' ) )
      break;

    if( str[index+21] != ' ' )
    { // line contains a validity value for a user ID

      // determine the validity
      Validity validity = KPGP_VALIDITY_UNKNOWN;
      if( !strncmp( str.data() + index+21, "complete", 8 ) )
        if( ultimateTrust )
          validity = KPGP_VALIDITY_ULTIMATE;
        else
          validity = KPGP_VALIDITY_FULL;
      else if( !strncmp( str.data() + index+21, "marginal", 8 ) )
        validity = KPGP_VALIDITY_MARGINAL;
      else if( !strncmp( str.data() + index+21, "never", 5 ) )
        validity = KPGP_VALIDITY_NEVER;
      else if( !strncmp( str.data() + index+21, "undefined", 9 ) )
        validity = KPGP_VALIDITY_UNDEFINED;

      // determine the user ID
      int pos = index + 31;
      if( str[index+2] == ' ' )
        pos++; // additional user IDs start one column later
      QString uid = str.mid( pos, index2-pos );

      // set the validity of the corresponding user ID
      for( UserIDList::Iterator it = userIDs.begin(); it != userIDs.end(); ++it )
        if( (*it)->text() == uid )
        {
          kDebug( 5326 )<<"Setting the validity of"<<uid<<" to"<<validity;
          (*it)->setValidity( validity );
          break;
        }
    }

    firstLine = false;
    index = index2 + 1;
  }
}


KeyList
Base2::parseKeyList( const QByteArray& output, bool secretKeys )
{
  kDebug( 5326 ) <<"Kpgp::Base2::parseKeyList()";
  KeyList keys;
  Key *key = 0;
  Subkey *subkey = 0;
  int index;

  // search start of key data
  if( !strncmp( output.data(), "pub", 3 ) ||
      !strncmp( output.data(), "sec", 3 ) )
    index = 0;
  else
  {
    if( secretKeys )
      index = output.indexOf( "\nsec" );
    else
      index = output.indexOf( "\npub" );
    if( index == -1 )
      return keys;
    else
      index++;
  }

  while( true )
  {
    int index2;

    // search the end of the current line
    if( ( index2 = output.indexOf( '\n', index ) ) == -1 )
      break;

    if( !strncmp( output.data() + index, "pub", 3 ) ||
        !strncmp( output.data() + index, "sec", 3 ) )
    { // line contains primary key data
      // Example 1:
      // pub  1024/E2D074D3 2001/09/09 Test Key <testkey@xyz>
      // Example 2 (disabled key):
      // pub- 1024/8CCB2C1B 2001/11/04 Disabled Test Key <disabled@xyz>
      // Example 3 (expired key):
      // pub> 1024/7B94827D 2001/09/09 Expired Test Key <expired@xyz> (EXPIRE:2001-09-10)
      // Example 4 (revoked key):
      // pub  1024/956721F9 2001/09/09 *** KEY REVOKED ***

      int pos, pos2;

      if( key != 0 ) // store the previous key in the key list
	keys.append( key );

      key = new Key();
      key->setSecret( secretKeys );
      // set default key capabilities
      key->setCanEncrypt( true );
      key->setCanSign( true );
      key->setCanCertify( true );

      subkey = new Subkey( "", secretKeys );
      key->addSubkey( subkey );
      // set default key capabilities
      subkey->setCanEncrypt( true );
      subkey->setCanSign( true );
      subkey->setCanCertify( true );
      // expiration date defaults to never
      subkey->setExpirationDate( -1 );

      // Key Flags
      switch( output[index+3] )
      {
      case ' ': // nothing special
        break;
      case '-': // disabled key
        subkey->setDisabled( true );
        key->setDisabled( true );
        break;
      case '>': // expired key
        subkey->setExpired( true );
        key->setExpired( true );
        break;
      default:
        kDebug( 5326 ) <<"Unknown key flag.";
      }

      // Key Length
      pos = index + 4;
      while( output[pos] == ' ' )
        pos++;
      pos2 = output.indexOf( '/', pos );
      subkey->setKeyLength( output.mid( pos, pos2-pos ).toUInt() );

      // Key ID
      pos = pos2 + 1;
      pos2 = output.indexOf( ' ', pos );
      subkey->setKeyID( output.mid( pos, pos2-pos ) );

      // Creation Date
      pos = pos2 + 1;
      while( output[pos] == ' ' )
        pos++;
      pos2 = output.indexOf( ' ', pos );
      int year = output.mid( pos, 4 ).toInt();
      int month = output.mid( pos+5, 2 ).toInt();
      int day = output.mid( pos+8, 2 ).toInt();
      QDateTime dt( QDate( year, month, day ), QTime( 00, 00 ) );
      QDateTime epoch( QDate( 1970, 01, 01 ), QTime( 00, 00 ) );
      // The calculated creation date isn't exactly correct because QDateTime
      // doesn't know anything about timezones and always assumes local time
      // although epoch is of course UTC. But as PGP 2 anyway doesn't print
      // the time this doesn't matter too much.
      subkey->setCreationDate( epoch.secsTo( dt ) );

      // User ID
      pos = pos2 + 1;
      while( output[pos] == ' ' )
        pos++;
      QByteArray uid = output.mid( pos, index2-pos );
      if( uid != "*** KEY REVOKED ***" )
        key->addUserID( uid );
      else
      {
        subkey->setRevoked( true );
        key->setRevoked( true );
      }
    }
    else if( output[index] == ' ' )
    { // line contains additional key data

      if( key == 0 )
        break;

      int pos = index + 1;
      while( output[pos] == ' ' )
        pos++;

      if( !strncmp( output.data() + pos, "Key fingerprint = ", 18 ) )
      { // line contains a fingerprint
        // Example:
        //             Key fingerprint = 47 30 7C 76 05 BF 5E FB  72 41 00 F2 7D 0B D0 49

        int pos2;
        pos2 = pos + 18;
        QByteArray fingerprint = output.mid( pos, index2-pos );
        // remove white space from the fingerprint
	for ( int idx = 0 ; (idx = fingerprint.indexOf(' ', idx)) != -1 ; )
	  fingerprint.replace( idx, 1, "" );

        subkey->setFingerprint( fingerprint );
      }
      else if( !strncmp( output.data() + pos, "Expire: ", 8 ) ||
               !strncmp( output.data() + pos, "no expire ", 10 ) )
      { // line contains additional key properties
        // Examples:
        //            Expire: 2001/09/10
        //                     no expire ENCRyption only
        //                     no expire SIGNature only

        if( output[pos] == 'E' )
        {
          // Expiration Date
          pos += 8;
          int year = output.mid( pos, 4 ).toInt();
          int month = output.mid( pos+5, 2 ).toInt();
          int day = output.mid( pos+8, 2 ).toInt();
          QDateTime dt( QDate( year, month, day ), QTime( 00, 00 ) );
          QDateTime epoch( QDate( 1970, 01, 01 ), QTime( 00, 00 ) );
          // Here the same comments as for the creation date are valid.
          subkey->setExpirationDate( epoch.secsTo( dt ) );
          pos += 11; // note that there is always a blank after the expire date
        }
        else
          pos += 10;

        // optional key capabilities (sign/encrypt only)
        if( pos != index2 )
        {
          if( !strncmp( output.data() + pos, "SIGNature only", 14 ) )
          {
            subkey->setCanEncrypt( false );
            key->setCanEncrypt( false );
          }
          else if( !strncmp( output.data() + pos, "ENCRyption only", 15 ) )
          {
            subkey->setCanSign( false );
            key->setCanSign( false );
            subkey->setCanCertify( false );
            key->setCanCertify( false );
          }
        }
      }
      else
      { // line contains an additional user id
        // Example:
        //                               Test key (2nd user ID) <abc@xyz>

        key->addUserID( output.mid( pos, index2-pos ) );
      }
    }

    index = index2 + 1;
  }

  if (key != 0) // store the last key in the key list
    keys.append( key );

  //kDebug( 5326 ) <<"finished parsing keys";

  return keys;
}


} // namespace Kpgp
