#pragma once

#include <bcrypt.h>

#pragma comment(lib, "Bcrypt.lib")

namespace my_crypt
{

class Hash
{

public:
    Hash(PCWSTR hid = BCRYPT_MD5_ALGORITHM)
    {
        m_hCrypt = NULL;
        m_hHash = NULL;

        ATLVERIFY(0 == ::BCryptOpenAlgorithmProvider(&m_hCrypt, hid, NULL, 0));
    }
    ~Hash()
    {
        Close();
 
        if (m_hCrypt)
        {
            ::BCryptCloseAlgorithmProvider(m_hCrypt, 0);
        }
    }
    bool Open()
    {
        return 0 == ::BCryptCreateHash(m_hCrypt, &m_hHash, NULL, 0, NULL, 0, 0);
    }
    void Close()
    {
        if (m_hHash)
        {
            ::BCryptDestroyHash(m_hHash);
            m_hHash = NULL;
        }
    }
    bool Update(const void* pbDataIn, ULONG cbDataIn)
    {
        ATLASSERT(m_hHash);
        return 0 == ::BCryptHashData(m_hHash, (PUCHAR)pbDataIn, cbDataIn, 0);
    }
    ULONG Commit(ATL::CTempBuffer<BYTE>& pBlob)
    {
        ATLASSERT(m_hHash);
        ULONG nHashLen = Size();

        if (!pBlob.Reallocate(nHashLen))
            return 0;

        if (0 == ::BCryptFinishHash(m_hHash, pBlob, nHashLen, 0))
            return nHashLen;
        return 0;
    }
    ULONG Size() const
    {
        ATLASSERT(m_hCrypt);

        ULONG r = 0;
        ULONG cb = 0;

        ::BCryptGetProperty(m_hCrypt, BCRYPT_HASH_LENGTH, (PUCHAR)&r, sizeof(r), &cb, 0);
        return r;
    }

public:
    BCRYPT_ALG_HANDLE m_hCrypt;
    BCRYPT_KEY_HANDLE m_hHash;
};

class Rsa
{
public:
    typedef enum
    {
        encrypt_oaep,
        decrypt_oaep,
        encrypt_pkcs1,
        decrypt_pkcs1
    } cryptMode;

public:
    Rsa()
    {
        m_hCrypt = NULL;
        m_hKey = NULL;

        ATLVERIFY(0 == ::BCryptOpenAlgorithmProvider(&m_hCrypt, BCRYPT_RSA_ALGORITHM, NULL, 0));
    }
    ~Rsa()
    {
        Close();

        if (m_hCrypt)
        {
            ::BCryptCloseAlgorithmProvider(m_hCrypt, 0);
        }
    }
    bool Open(const BCRYPT_RSAKEY_BLOB* pKeyBlob, ULONG nBlobLen)
    {
        bool r = false;

        ATLASSERT(pKeyBlob && nBlobLen);

        Close();

        if (m_hCrypt)
        {
            switch (pKeyBlob->Magic)
            {
                case BCRYPT_RSAFULLPRIVATE_MAGIC:
                {
                    r = 0 == ::BCryptImportKeyPair(m_hCrypt, NULL, BCRYPT_RSAFULLPRIVATE_BLOB, &m_hKey, (PUCHAR)pKeyBlob, nBlobLen, 0);
                }
                break;

                case BCRYPT_RSAPRIVATE_MAGIC:
                {
                    r = 0 == ::BCryptImportKeyPair(m_hCrypt, NULL, BCRYPT_RSAPRIVATE_BLOB, &m_hKey, (PUCHAR)pKeyBlob, nBlobLen, 0);
                }
                break;

                case BCRYPT_RSAPUBLIC_MAGIC:
                {
                    r = 0 == ::BCryptImportKeyPair(m_hCrypt, NULL, BCRYPT_RSAPUBLIC_BLOB, &m_hKey, (PUCHAR)pKeyBlob, nBlobLen, 0);
                }
                break;
            }
        }
        return r;
    }
    inline bool Open(const BYTE* pBlob, ULONG nBlobLen)
    {
        return Open((const BCRYPT_RSAKEY_BLOB*)pBlob, nBlobLen);
    }
    bool Crypt(cryptMode nMode, PUCHAR pbDataIn, ULONG cbDataIn, ATL::CTempBuffer<BYTE>& blobOut, ULONG& cbDataOut, PCWSTR strPaddingHashId = BCRYPT_SHA512_ALGORITHM)
    {
        if (cbDataIn == 0)
        {
            cbDataOut = 0;
            return true;
        }

        bool r = false;

        ULONG cbOutput = 0;
        cbDataOut = 0;

        BCRYPT_OAEP_PADDING_INFO oaep_paddingInfo;

        memset(&oaep_paddingInfo, 0, sizeof(oaep_paddingInfo));
        oaep_paddingInfo.pszAlgId = strPaddingHashId;

        switch (nMode)
        {
            case encrypt_oaep:
            {
                r = 0 == ::BCryptEncrypt(m_hKey, pbDataIn, cbDataIn, &oaep_paddingInfo, NULL, 0, NULL, 0, &cbOutput, BCRYPT_PAD_OAEP);
            }
            break;

            case decrypt_oaep:
            {
                r = 0 == ::BCryptDecrypt(m_hKey, pbDataIn, cbDataIn, &oaep_paddingInfo, NULL, 0, NULL, 0, &cbOutput, BCRYPT_PAD_OAEP);
            }
            break;

            case encrypt_pkcs1:
            {
                r = 0 == ::BCryptEncrypt(m_hKey, pbDataIn, cbDataIn, NULL, NULL, 0, NULL, 0, &cbOutput, BCRYPT_PAD_PKCS1);
            }
            break;

            case decrypt_pkcs1:
            {
                r = 0 == ::BCryptDecrypt(m_hKey, pbDataIn, cbDataIn, NULL, NULL, 0, NULL, 0, &cbOutput, BCRYPT_PAD_PKCS1);
            }
            break;
        }
        if (!r)
        {
            return false;
        }
        if (!blobOut.Reallocate(cbOutput))
            return false;

        switch (nMode)
        {
            case encrypt_oaep:
            {
                r = 0 == ::BCryptEncrypt(m_hKey, pbDataIn, cbDataIn, &oaep_paddingInfo, NULL, 0, blobOut, cbOutput, &cbDataOut, BCRYPT_PAD_OAEP);
            }
            break;

            case decrypt_oaep:
            {
                r = 0 == ::BCryptDecrypt(m_hKey, pbDataIn, cbDataIn, &oaep_paddingInfo, NULL, 0, blobOut, cbOutput, &cbDataOut, BCRYPT_PAD_OAEP);
            }
            break;

            case encrypt_pkcs1:
            {
                r = 0 == ::BCryptEncrypt(m_hKey, pbDataIn, cbDataIn, NULL, NULL, 0, blobOut, cbOutput, &cbDataOut, BCRYPT_PAD_PKCS1);
            }
            break;

            case decrypt_pkcs1:
            {
                r = 0 == ::BCryptDecrypt(m_hKey, pbDataIn, cbDataIn, NULL, NULL, 0, blobOut, cbOutput, &cbDataOut, BCRYPT_PAD_PKCS1);
            }
            break;
        }
        return r;
    }
    bool Sign(PCWSTR strHashId, PUCHAR pbDataIn, ULONG cbDataIn, CTempBuffer<BYTE>& blobOut, ULONG& cbDataOut)
    {
        BCRYPT_PKCS1_PADDING_INFO pkcs1_paddingInfo;

        memset(&pkcs1_paddingInfo, 0, sizeof(pkcs1_paddingInfo));
        pkcs1_paddingInfo.pszAlgId = strHashId;

        ULONG cbOutput = 0;
        cbDataOut = 0;

        if (0 != ::BCryptSignHash(m_hKey, &pkcs1_paddingInfo, pbDataIn, cbDataIn, NULL, 0, &cbOutput, BCRYPT_PAD_PKCS1))
        {
            return false;
        }
        if (!blobOut.Reallocate(cbOutput))
            return false;

        if (0 != ::BCryptSignHash(m_hKey, &pkcs1_paddingInfo, pbDataIn, cbDataIn, blobOut, cbOutput, &cbDataOut, BCRYPT_PAD_PKCS1))
        {
            return false;
        }
        return true;
    }
    void Close()
    {
        if (m_hKey)
        {
            ATLVERIFY(0 == ::BCryptDestroyKey(m_hKey));
            m_hKey = NULL;
        }
    }

public:
    BCRYPT_ALG_HANDLE m_hCrypt;
    BCRYPT_KEY_HANDLE m_hKey;
};

class Aes
{
public:
    Aes()
    {
        m_hCrypt = NULL;
        m_hKey = NULL;
        m_IvLen = 0;
        m_KeyLen = 0;

        ATLVERIFY(0 == ::BCryptOpenAlgorithmProvider(&m_hCrypt, BCRYPT_AES_ALGORITHM, NULL, 0));
    }
    ~Aes()
    {
        Close();

        if (m_hCrypt)
        {
            ::BCryptCloseAlgorithmProvider(m_hCrypt, 0);
        }
    }
    bool Open(const void* pKey, ULONG nKeyLen, const void* pIv = NULL, ULONG nIvLen = 0)
    {
        bool r = false;

        if (0 != ::BCryptSetProperty(m_hCrypt, BCRYPT_CHAINING_MODE, (PUCHAR)BCRYPT_CHAIN_MODE_CBC, sizeof(BCRYPT_CHAIN_MODE_CBC), 0))
        {
            return false;
        }
        ULONG nData = 0;

        if (0 != ::BCryptGetProperty(m_hCrypt, BCRYPT_OBJECT_LENGTH, (PUCHAR)&m_KeyLen, sizeof(m_KeyLen), &nData, 0))
        {
            return false;
        }
        ATLASSERT(nData == sizeof(ULONG));

        if (m_Key.Reallocate(m_KeyLen) && (nIvLen == 0 || m_Iv.Reallocate(nIvLen)))
        {
            m_IvLen = nIvLen;

            if (m_IvLen)
                memcpy(m_Iv, pIv, nIvLen);

            ATL::CTempBuffer<BCRYPT_KEY_DATA_BLOB_HEADER> keyBlob;

            if (keyBlob.AllocateBytes(sizeof(BCRYPT_KEY_DATA_BLOB_HEADER) + nKeyLen))
            {
                memset(keyBlob, 0, sizeof(BCRYPT_KEY_DATA_BLOB_HEADER) + nKeyLen);

                keyBlob->dwMagic = BCRYPT_KEY_DATA_BLOB_MAGIC;
                keyBlob->dwVersion = BCRYPT_KEY_DATA_BLOB_VERSION1;
                keyBlob->cbKeyData = nKeyLen;

                memcpy(keyBlob + 1, pKey, nKeyLen);

                r = 0 == ::BCryptImportKey(m_hCrypt, NULL, BCRYPT_KEY_DATA_BLOB, &m_hKey, m_Key, m_KeyLen
                                    , (PUCHAR)(BCRYPT_KEY_DATA_BLOB_HEADER*)keyBlob, sizeof(BCRYPT_KEY_DATA_BLOB_HEADER) + nKeyLen, 0);
            }
        }
        if (!r)
            Close();
        return r;
    }
    void Close()
    {
        if (m_hKey)
        {
            ::BCryptDestroyKey(m_hKey);
            m_hKey = NULL;
        }
        m_IvLen = 0;
        m_KeyLen = 0;
    }
    ULONG Encrypt(const void* in, unsigned char* out, ULONG bytes, BYTE* pIv = NULL, ULONG nIv = 0)
    {
        if (!out || !bytes)
            return 0;

        if (!pIv)
        {
            pIv = m_Iv;
            nIv = m_IvLen;
        }
        ULONG r = 0;

        ::BCryptEncrypt(m_hKey, (PUCHAR)in, bytes, NULL, pIv, nIv, out, bytes, &r, 0);

        return r;
    }
    ULONG Decrypt(const void* in, unsigned char* out, ULONG bytes, BYTE* pIv = NULL, ULONG nIv = 0)
    {
        if (!out || !bytes)
            return 0;

        if (!pIv)
        {
            pIv = m_Iv;
            nIv = m_IvLen;
        }
        ULONG r = 0;

        ::BCryptDecrypt(m_hKey, (PUCHAR)in, bytes, NULL, pIv, nIv, out, bytes, &r, 0);

        return r;
    }
public:
    BCRYPT_ALG_HANDLE           m_hCrypt;
    BCRYPT_KEY_HANDLE           m_hKey;
    ATL::CTempBuffer<BYTE>      m_Key;
    ULONG                       m_KeyLen;
    ATL::CTempBuffer<BYTE>      m_Iv;
    ULONG                       m_IvLen;
};

}