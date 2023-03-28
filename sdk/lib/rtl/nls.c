/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * FILE:              lib/rtl/nls.c
 * PURPOSE:           National Language Support (NLS) functions
 * PROGRAMMERS:       Emanuele Aliberti
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

PUSHORT NlsUnicodeUpcaseTable = NULL;
PUSHORT NlsUnicodeLowercaseTable = NULL;

USHORT NlsAnsiCodePage = 0; /* exported */
BOOLEAN NlsMbCodePageTag = FALSE; /* exported */
PUSHORT NlsAnsiToUnicodeTable = NULL;
PCHAR NlsUnicodeToAnsiTable = NULL;
PUSHORT NlsUnicodeToMbAnsiTable = NULL;
PUSHORT NlsLeadByteInfo = NULL; /* exported */

USHORT NlsOemCodePage = 0;
BOOLEAN NlsMbOemCodePageTag = FALSE; /* exported */
PUSHORT NlsOemToUnicodeTable = NULL;
PCHAR NlsUnicodeToOemTable = NULL;
PUSHORT NlsUnicodeToMbOemTable = NULL;
PUSHORT NlsOemLeadByteInfo = NULL; /* exported */

USHORT NlsOemDefaultChar = '\0';
USHORT NlsUnicodeDefaultChar = 0;


/* FUNCTIONS *****************************************************************/

/*
 * @unimplemented
 */
NTSTATUS NTAPI
RtlCustomCPToUnicodeN(IN PCPTABLEINFO CustomCP,
                      OUT PWCHAR UnicodeString,
                      IN ULONG UnicodeSize,
                      OUT PULONG ResultSize OPTIONAL,
                      IN PCHAR CustomString,
                      IN ULONG CustomSize)
{
    ULONG Size = 0;
    ULONG i;

    PAGED_CODE_RTL();

    if (!CustomCP->DBCSCodePage)
    {
        /* single-byte code page */
        if (CustomSize > (UnicodeSize / sizeof(WCHAR)))
            Size = UnicodeSize / sizeof(WCHAR);
        else
            Size = CustomSize;

        if (ResultSize)
            *ResultSize = Size * sizeof(WCHAR);

        for (i = 0; i < Size; i++)
        {
            *UnicodeString = CustomCP->MultiByteTable[(UCHAR)*CustomString];
            UnicodeString++;
            CustomString++;
        }
    }
    else
    {
        /* multi-byte code page */
        /* FIXME */
        ASSERT(FALSE);
    }

    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
WCHAR NTAPI
RtlpDowncaseUnicodeChar(IN WCHAR Source)
{
    USHORT Offset;

    PAGED_CODE_RTL();

    if (Source < L'A')
        return Source;

    if (Source <= L'Z')
        return Source + (L'a' - L'A');

    if (Source < 0x80)
        return Source;

    Offset = ((USHORT)Source >> 8);
    DPRINT("Offset: %hx\n", Offset);

    Offset = NlsUnicodeLowercaseTable[Offset];
    DPRINT("Offset: %hx\n", Offset);

    Offset += (((USHORT)Source & 0x00F0) >> 4);
    DPRINT("Offset: %hx\n", Offset);

    Offset = NlsUnicodeLowercaseTable[Offset];
    DPRINT("Offset: %hx\n", Offset);

    Offset += ((USHORT)Source & 0x000F);
    DPRINT("Offset: %hx\n", Offset);

    Offset = NlsUnicodeLowercaseTable[Offset];
    DPRINT("Offset: %hx\n", Offset);

    DPRINT("Result: %hx\n", Source + (SHORT)Offset);

    return Source + (SHORT)Offset;
}

/*
 * @implemented
 */
WCHAR NTAPI
RtlDowncaseUnicodeChar(IN WCHAR Source)
{
    PAGED_CODE_RTL();

    return RtlpDowncaseUnicodeChar(Source);
}

/*
 * @implemented
 */
VOID NTAPI
RtlGetDefaultCodePage(OUT PUSHORT AnsiCodePage,
                      OUT PUSHORT OemCodePage)
{
    PAGED_CODE_RTL();

    *AnsiCodePage = NlsAnsiCodePage;
    *OemCodePage = NlsOemCodePage;
}

/*
 * @implemented
 */
VOID NTAPI
RtlInitCodePageTable(IN PUSHORT TableBase,
                     OUT PCPTABLEINFO CodePageTable)
{
    PNLS_FILE_HEADER NlsFileHeader;

    PAGED_CODE_RTL();

    DPRINT("RtlInitCodePageTable() called\n");

    NlsFileHeader = (PNLS_FILE_HEADER)TableBase;

    /* Copy header fields first */
    CodePageTable->CodePage = NlsFileHeader->CodePage;
    CodePageTable->MaximumCharacterSize = NlsFileHeader->MaximumCharacterSize;
    CodePageTable->DefaultChar = NlsFileHeader->DefaultChar;
    CodePageTable->UniDefaultChar = NlsFileHeader->UniDefaultChar;
    CodePageTable->TransDefaultChar = NlsFileHeader->TransDefaultChar;
    CodePageTable->TransUniDefaultChar = NlsFileHeader->TransUniDefaultChar;

    RtlCopyMemory(&CodePageTable->LeadByte,
                  &NlsFileHeader->LeadByte,
                  MAXIMUM_LEADBYTES);

    /* Offset to wide char table is after the header */
    CodePageTable->WideCharTable =
        TableBase + NlsFileHeader->HeaderSize + 1 + TableBase[NlsFileHeader->HeaderSize];

    /* Then multibyte table (256 wchars) follows */
    CodePageTable->MultiByteTable = TableBase + NlsFileHeader->HeaderSize + 1;

    /* Check the presence of glyph table (256 wchars) */
    if (!CodePageTable->MultiByteTable[256])
        CodePageTable->DBCSRanges = CodePageTable->MultiByteTable + 256 + 1;
    else
        CodePageTable->DBCSRanges = CodePageTable->MultiByteTable + 256 + 1 + 256;

    /* Is this double-byte code page? */
    if (*CodePageTable->DBCSRanges)
    {
        CodePageTable->DBCSCodePage = 1;
        CodePageTable->DBCSOffsets = CodePageTable->DBCSRanges + 1;
    }
    else
    {
        CodePageTable->DBCSCodePage = 0;
        CodePageTable->DBCSOffsets = NULL;
    }
}

/*
 * @implemented
 */
VOID NTAPI
RtlInitNlsTables(IN PUSHORT AnsiTableBase,
                 IN PUSHORT OemTableBase,
                 IN PUSHORT CaseTableBase,
                 OUT PNLSTABLEINFO NlsTable)
{
    PAGED_CODE_RTL();

    DPRINT("RtlInitNlsTables()called\n");

    if (AnsiTableBase && OemTableBase && CaseTableBase)
    {
        RtlInitCodePageTable(AnsiTableBase, &NlsTable->AnsiTableInfo);
        RtlInitCodePageTable(OemTableBase, &NlsTable->OemTableInfo);

        NlsTable->UpperCaseTable = (PUSHORT)CaseTableBase + 2;
        NlsTable->LowerCaseTable = (PUSHORT)CaseTableBase + *((PUSHORT)CaseTableBase + 1) + 2;
    }
}

/*
 * @unimplemented
 */
NTSTATUS NTAPI
RtlMultiByteToUnicodeN(OUT PWCHAR UnicodeString,
                       IN ULONG UnicodeSize,
                       OUT PULONG ResultSize,
                       IN PCSTR MbString,
                       IN ULONG MbSize)
{
    ULONG Size = 0;
    ULONG i;

    PAGED_CODE_RTL();

    if (!NlsMbCodePageTag)
    {
        /* single-byte code page */
        if (MbSize > (UnicodeSize / sizeof(WCHAR)))
            Size = UnicodeSize / sizeof(WCHAR);
        else
            Size = MbSize;

        if (ResultSize)
            *ResultSize = Size * sizeof(WCHAR);

        for (i = 0; i < Size; i++)
        {
            UnicodeString[i] = NlsAnsiToUnicodeTable[(UCHAR)MbString[i]];
        }
    }
    else
    {
        /* multi-byte code page */
        /* FIXME */

        UCHAR Char;
        USHORT LeadByteInfo;
        PCSTR MbEnd = MbString + MbSize;

        for (i = 0; i < UnicodeSize / sizeof(WCHAR) && MbString < MbEnd; i++)
        {
            Char = *(PUCHAR)MbString++;

            if (Char < 0x80)
            {
                *UnicodeString++ = Char;
                continue;
            }

            LeadByteInfo = NlsLeadByteInfo[Char];

            if (!LeadByteInfo)
            {
                *UnicodeString++ = NlsAnsiToUnicodeTable[Char];
                continue;
            }

            if (MbString < MbEnd)
                *UnicodeString++ = NlsLeadByteInfo[LeadByteInfo + *(PUCHAR)MbString++];
        }

        if (ResultSize)
            *ResultSize = i * sizeof(WCHAR);
    }

    return STATUS_SUCCESS;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
RtlConsoleMultiByteToUnicodeN(OUT PWCHAR UnicodeString,
                              IN ULONG UnicodeSize,
                              OUT PULONG ResultSize,
                              IN PCSTR MbString,
                              IN ULONG MbSize,
                              OUT PULONG Unknown)
{
    PAGED_CODE_RTL();

    UNIMPLEMENTED;
    DPRINT1("RtlConsoleMultiByteToUnicodeN calling RtlMultiByteToUnicodeN\n");
    *Unknown = 1;
    return RtlMultiByteToUnicodeN(UnicodeString,
                                  UnicodeSize,
                                  ResultSize,
                                  MbString,
                                  MbSize);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlMultiByteToUnicodeSize(OUT PULONG UnicodeSize,
                          IN PCSTR MbString,
                          IN ULONG MbSize)
{
    ULONG Length = 0;

    PAGED_CODE_RTL();

    if (!NlsMbCodePageTag)
    {
        /* single-byte code page */
        *UnicodeSize = MbSize * sizeof(WCHAR);
    }
    else
    {
        /* multi-byte code page */
        /* FIXME */

        while (MbSize--)
        {
            UCHAR Char = *(PUCHAR)MbString++;

            if (Char >= 0x80 && NlsLeadByteInfo[Char])
            {
                if (MbSize)
                {
                    /* Move on */
                    MbSize--;
                    MbString++;
                }
            }

            /* Increase returned size */
            Length++;
        }

        /* Return final size */
        *UnicodeSize = Length * sizeof(WCHAR);
    }

    /* Success */
    return STATUS_SUCCESS;
}

/*
 * @unimplemented
 */
NTSTATUS NTAPI
RtlOemToUnicodeN(OUT PWCHAR UnicodeString,
                 IN ULONG UnicodeSize,
                 OUT PULONG ResultSize OPTIONAL,
                 IN PCCH OemString,
                 IN ULONG OemSize)
{
    ULONG Size = 0;
    ULONG i;

    PAGED_CODE_RTL();

    if (!NlsMbOemCodePageTag)
    {
        /* single-byte code page */
        if (OemSize > (UnicodeSize / sizeof(WCHAR)))
            Size = UnicodeSize / sizeof(WCHAR);
        else
            Size = OemSize;

        if (ResultSize)
            *ResultSize = Size * sizeof(WCHAR);

        for (i = 0; i < Size; i++)
        {
            *UnicodeString = NlsOemToUnicodeTable[(UCHAR)*OemString];
            UnicodeString++;
            OemString++;
        }
    }
    else
    {
        /* multi-byte code page */
        /* FIXME */

        UCHAR Char;
        USHORT OemLeadByteInfo;
        PCCH OemEnd = OemString + OemSize;

        for (i = 0; i < UnicodeSize / sizeof(WCHAR) && OemString < OemEnd; i++)
        {
            Char = *(PUCHAR)OemString++;

            if (Char < 0x80)
            {
                *UnicodeString++ = Char;
                continue;
            }

            OemLeadByteInfo = NlsOemLeadByteInfo[Char];

            if (!OemLeadByteInfo)
            {
                *UnicodeString++ = NlsOemToUnicodeTable[Char];
                continue;
            }

            if (OemString < OemEnd)
                *UnicodeString++ =
                    NlsOemLeadByteInfo[OemLeadByteInfo + *(PUCHAR)OemString++];
        }

        if (ResultSize)
            *ResultSize = i * sizeof(WCHAR);
    }

    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
VOID NTAPI
RtlResetRtlTranslations(IN PNLSTABLEINFO NlsTable)
{
    PAGED_CODE_RTL();

    DPRINT("RtlResetRtlTranslations() called\n");

    /* Set ANSI data */
    NlsAnsiToUnicodeTable = (PUSHORT)NlsTable->AnsiTableInfo.MultiByteTable;
    NlsUnicodeToAnsiTable = NlsTable->AnsiTableInfo.WideCharTable;
    NlsUnicodeToMbAnsiTable = (PUSHORT)NlsTable->AnsiTableInfo.WideCharTable;
    NlsMbCodePageTag = (NlsTable->AnsiTableInfo.DBCSCodePage != 0);
    NlsLeadByteInfo = NlsTable->AnsiTableInfo.DBCSOffsets;
    NlsAnsiCodePage = NlsTable->AnsiTableInfo.CodePage;
    DPRINT("Ansi codepage %hu\n", NlsAnsiCodePage);

    /* Set OEM data */
    NlsOemToUnicodeTable = (PUSHORT)NlsTable->OemTableInfo.MultiByteTable;
    NlsUnicodeToOemTable = NlsTable->OemTableInfo.WideCharTable;
    NlsUnicodeToMbOemTable = (PUSHORT)NlsTable->OemTableInfo.WideCharTable;
    NlsMbOemCodePageTag = (NlsTable->OemTableInfo.DBCSCodePage != 0);
    NlsOemLeadByteInfo = NlsTable->OemTableInfo.DBCSOffsets;
    NlsOemCodePage = NlsTable->OemTableInfo.CodePage;
    DPRINT("Oem codepage %hu\n", NlsOemCodePage);

    /* Set Unicode case map data */
    NlsUnicodeUpcaseTable = NlsTable->UpperCaseTable;
    NlsUnicodeLowercaseTable = NlsTable->LowerCaseTable;

    /* set the default characters for RtlpDidUnicodeToOemWork */
    NlsOemDefaultChar = NlsTable->OemTableInfo.DefaultChar;
    NlsUnicodeDefaultChar = NlsTable->OemTableInfo.TransDefaultChar;
}

/*
 * @unimplemented
 */
NTSTATUS NTAPI
RtlUnicodeToCustomCPN(IN PCPTABLEINFO CustomCP,
                      OUT PCHAR CustomString,
                      IN ULONG CustomSize,
                      OUT PULONG ResultSize OPTIONAL,
                      IN PWCHAR UnicodeString,
                      IN ULONG UnicodeSize)
{
    ULONG Size = 0;
    ULONG i;

    PAGED_CODE_RTL();

    if (!CustomCP->DBCSCodePage)
    {
        /* single-byte code page */
        if (UnicodeSize > (CustomSize * sizeof(WCHAR)))
            Size = CustomSize;
        else
            Size = UnicodeSize / sizeof(WCHAR);

        if (ResultSize)
            *ResultSize = Size;

        for (i = 0; i < Size; i++)
        {
            *CustomString = ((PCHAR)CustomCP->WideCharTable)[*UnicodeString];
            CustomString++;
            UnicodeString++;
        }
    }
    else
    {
        /* multi-byte code page */
        /* FIXME */
        ASSERT(FALSE);
    }

    return STATUS_SUCCESS;
}

/*
 * @unimplemented
 */
NTSTATUS NTAPI
RtlUnicodeToMultiByteN(OUT PCHAR MbString,
                       IN ULONG MbSize,
                       OUT PULONG ResultSize OPTIONAL,
                       IN PCWCH UnicodeString,
                       IN ULONG UnicodeSize)
{
    ULONG Size = 0;
    ULONG i;

    PAGED_CODE_RTL();

    if (!NlsMbCodePageTag)
    {
        /* single-byte code page */
        Size = (UnicodeSize > (MbSize * sizeof(WCHAR)))
                ? MbSize : (UnicodeSize / sizeof(WCHAR));

        if (ResultSize)
            *ResultSize = Size;

        for (i = 0; i < Size; i++)
        {
            *MbString++ = NlsUnicodeToAnsiTable[*UnicodeString++];
        }
    }
    else
    {
        /* multi-byte code page */
        /* FIXME */

        USHORT WideChar;
        USHORT MbChar;

        for (i = MbSize, Size = UnicodeSize / sizeof(WCHAR); i && Size; i--, Size--)
        {
            WideChar = *UnicodeString++;

            if (WideChar < 0x80)
            {
                *MbString++ = LOBYTE(WideChar);
                continue;
            }

            MbChar = NlsUnicodeToMbAnsiTable[WideChar];

            if (!HIBYTE(MbChar))
            {
                *MbString++ = LOBYTE(MbChar);
                continue;
            }

            if (i >= 2)
            {
                *MbString++ = HIBYTE(MbChar);
                *MbString++ = LOBYTE(MbChar);
                i--;
            }
            else break;
        }

        if (ResultSize)
            *ResultSize = MbSize - i;
    }

    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlUnicodeToMultiByteSize(OUT PULONG MbSize,
                          IN PCWCH UnicodeString,
                          IN ULONG UnicodeSize)
{
    ULONG UnicodeLength = UnicodeSize / sizeof(WCHAR);
    ULONG MbLength = 0;

    PAGED_CODE_RTL();

    if (!NlsMbCodePageTag)
    {
        /* single-byte code page */
        *MbSize = UnicodeLength;
    }
    else
    {
        /* multi-byte code page */
        /* FIXME */

        while (UnicodeLength--)
        {
            USHORT WideChar = *UnicodeString++;

            if (WideChar >= 0x80 && HIBYTE(NlsUnicodeToMbAnsiTable[WideChar]))
            {
                MbLength += sizeof(WCHAR);
            }
            else
            {
                MbLength++;
            }
        }

        *MbSize = MbLength;
    }

    /* Success */
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlUnicodeToOemN(OUT PCHAR OemString,
                 IN ULONG OemSize,
                 OUT PULONG ResultSize OPTIONAL,
                 IN PCWCH UnicodeString,
                 IN ULONG UnicodeSize)
{
    ULONG Size = 0;

    PAGED_CODE_RTL();

    /* Bytes -> chars */
    UnicodeSize /= sizeof(WCHAR);

    if (!NlsMbOemCodePageTag)
    {
        while (OemSize && UnicodeSize)
        {
            OemString[Size] = NlsUnicodeToOemTable[*UnicodeString++];
            Size++;
            OemSize--;
            UnicodeSize--;
        }
    }
    else
    {
        while (OemSize && UnicodeSize)
        {
            USHORT OemChar = NlsUnicodeToMbOemTable[*UnicodeString++];

            if (HIBYTE(OemChar))
            {
                if (OemSize < 2)
                    break;
                OemString[Size++] = HIBYTE(OemChar);
                OemSize--;
            }
            OemString[Size++] = LOBYTE(OemChar);
            OemSize--;
            UnicodeSize--;
        }
    }

    if (ResultSize)
        *ResultSize = Size;

    return UnicodeSize ? STATUS_BUFFER_OVERFLOW : STATUS_SUCCESS;
}

/*
 * @implemented
 */
WCHAR NTAPI
RtlpUpcaseUnicodeChar(IN WCHAR Source)
{
    USHORT Offset;

    if (Source < 'a')
        return Source;

    if (Source <= 'z')
        return (Source - ('a' - 'A'));

    Offset = ((USHORT)Source >> 8) & 0xFF;
    Offset = NlsUnicodeUpcaseTable[Offset];

    Offset += ((USHORT)Source >> 4) & 0xF;
    Offset = NlsUnicodeUpcaseTable[Offset];

    Offset += ((USHORT)Source & 0xF);
    Offset = NlsUnicodeUpcaseTable[Offset];

    return Source + (SHORT)Offset;
}

/*
 * @implemented
 */
WCHAR NTAPI
RtlUpcaseUnicodeChar(IN WCHAR Source)
{
    PAGED_CODE_RTL();

    return RtlpUpcaseUnicodeChar(Source);
}

/*
 * @implemented
 */
NTSTATUS NTAPI
RtlUpcaseUnicodeToCustomCPN(IN PCPTABLEINFO CustomCP,
                            OUT PCHAR CustomString,
                            IN ULONG CustomSize,
                            OUT PULONG ResultSize OPTIONAL,
                            IN PWCHAR UnicodeString,
                            IN ULONG UnicodeSize)
{
    WCHAR UpcaseChar;
    ULONG Size = 0;
    ULONG i;

    PAGED_CODE_RTL();

    if (!CustomCP->DBCSCodePage)
    {
        /* single-byte code page */
        if (UnicodeSize > (CustomSize * sizeof(WCHAR)))
            Size = CustomSize;
        else
            Size = UnicodeSize / sizeof(WCHAR);

        if (ResultSize)
            *ResultSize = Size;

        for (i = 0; i < Size; i++)
        {
            UpcaseChar = RtlpUpcaseUnicodeChar(*UnicodeString);
            *CustomString = ((PCHAR)CustomCP->WideCharTable)[UpcaseChar];
            ++CustomString;
            ++UnicodeString;
        }
    }
    else
    {
        /* multi-byte code page */
        /* FIXME */
        ASSERT(FALSE);
    }

    return STATUS_SUCCESS;
}

/*
 * @unimplemented
 */
NTSTATUS NTAPI
RtlUpcaseUnicodeToMultiByteN(OUT PCHAR MbString,
                             IN ULONG MbSize,
                             OUT PULONG ResultSize OPTIONAL,
                             IN PCWCH UnicodeString,
                             IN ULONG UnicodeSize)
{
    WCHAR UpcaseChar;
    ULONG Size = 0;
    ULONG i;

    PAGED_CODE_RTL();

    if (!NlsMbCodePageTag)
    {
        /* single-byte code page */
        if (UnicodeSize > (MbSize * sizeof(WCHAR)))
            Size = MbSize;
        else
            Size = UnicodeSize / sizeof(WCHAR);

        if (ResultSize)
            *ResultSize = Size;

        for (i = 0; i < Size; i++)
        {
            UpcaseChar = RtlpUpcaseUnicodeChar(*UnicodeString);
            *MbString = NlsUnicodeToAnsiTable[UpcaseChar];
            MbString++;
            UnicodeString++;
        }
    }
    else
    {
        /* multi-byte code page */
        /* FIXME */
        ASSERT(FALSE);
    }

    return STATUS_SUCCESS;
}

/*
 * @unimplemented
 */
NTSTATUS NTAPI
RtlUpcaseUnicodeToOemN(OUT PCHAR OemString,
                       IN ULONG OemSize,
                       OUT PULONG ResultSize OPTIONAL,
                       IN PCWCH UnicodeString,
                       IN ULONG UnicodeSize)
{
    WCHAR UpcaseChar;
    ULONG Size = 0;
    ULONG i;

    PAGED_CODE_RTL();

    if (!NlsMbOemCodePageTag)
    {
        /* single-byte code page */
        if (UnicodeSize > (OemSize * sizeof(WCHAR)))
            Size = OemSize;
        else
            Size = UnicodeSize / sizeof(WCHAR);

        if (ResultSize)
            *ResultSize = Size;

        for (i = 0; i < Size; i++)
        {
            UpcaseChar = RtlpUpcaseUnicodeChar(*UnicodeString);
            *OemString = NlsUnicodeToOemTable[UpcaseChar];
            OemString++;
            UnicodeString++;
        }
    }
    else
    {
        /* multi-byte code page */
        /* FIXME */

        USHORT WideChar;
        USHORT OemChar;

        for (i = OemSize, Size = UnicodeSize / sizeof(WCHAR); i && Size; i--, Size--)
        {
            WideChar = RtlpUpcaseUnicodeChar(*UnicodeString++);

            if (WideChar < 0x80)
            {
                *OemString++ = LOBYTE(WideChar);
                continue;
            }

            OemChar = NlsUnicodeToMbOemTable[WideChar];

            if (!HIBYTE(OemChar))
            {
                *OemString++ = LOBYTE(OemChar);
                continue;
            }

            if (i >= 2)
            {
                *OemString++ = HIBYTE(OemChar);
                *OemString++ = LOBYTE(OemChar);
                i--;
            }
            else break;
        }

        if (ResultSize)
            *ResultSize = OemSize - i;
    }

    return STATUS_SUCCESS;
}

/*
 * @unimplemented
 */
CHAR NTAPI
RtlUpperChar(IN CHAR Source)
{
    WCHAR Unicode;
    CHAR Destination;

    PAGED_CODE_RTL();

    /* Check for simple ANSI case */
    if (Source <= 'z')
    {
        /* Check for simple downcase a-z case */
        if (Source >= 'a')
        {
            /* Just XOR with the difference */
            return Source ^ ('a' - 'A');
        }
        else
        {
            /* Otherwise return the same char, it's already upcase */
            return Source;
        }
    }
    else
    {
        if (!NlsMbCodePageTag)
        {
            /* single-byte code page */

            /* ansi->unicode */
            Unicode = NlsAnsiToUnicodeTable[(UCHAR)Source];

            /* upcase conversion */
            Unicode = RtlpUpcaseUnicodeChar (Unicode);

            /* unicode -> ansi */
            Destination = NlsUnicodeToAnsiTable[(USHORT)Unicode];
        }
        else
        {
            /* multi-byte code page */
            /* FIXME */
            Destination = Source;
        }
    }

    return Destination;
}

 /* MUI defines. */
 #define MUI_LANGUAGE_ID                     0x04
 #define MUI_LANGUAGE_NAME                   0x08
 #define MUI_MERGE_SYSTEM_FALLBACK           0x10
 #define MUI_MERGE_USER_FALLBACK             0x20
 #define MUI_UI_FALLBACK                     MUI_MERGE_SYSTEM_FALLBACK | MUI_MERGE_USER_FALLBACK
 #define MUI_THREAD_LANGUAGES                0x40
 #define MUI_CONSOLE_FILTER                  0x100
 #define MUI_COMPLEX_SCRIPT_FILTER           0x200
 #define MUI_RESET_FILTERS                   0x001
 #define MUI_USER_PREFERRED_UI_LANGUAGES     0x10
 #define MUI_USE_INSTALLED_LANGUAGES         0x20
 #define MUI_USE_SEARCH_ALL_LANGUAGES        0x40
 #define MUI_LANG_NEUTRAL_PE_FILE            0x100
 #define MUI_NON_LANG_NEUTRAL_FILE           0x200
 #define MUI_MACHINE_LANGUAGE_SETTINGS       0x400
 #define MUI_FILETYPE_NOT_LANGUAGE_NEUTRAL   0x001
 #define MUI_FILETYPE_LANGUAGE_NEUTRAL_MAIN  0x002
 #define MUI_FILETYPE_LANGUAGE_NEUTRAL_MUI   0x004
 #define MUI_QUERY_TYPE                      0x001
 #define MUI_QUERY_CHECKSUM                  0x002
 #define MUI_QUERY_LANGUAGE_NAME             0x004
 #define MUI_QUERY_RESOURCE_TYPES            0x008
 #define MUI_FILEINFO_VERSION                0x001
 #define MUI_FULL_LANGUAGE                   0x01
 #define MUI_PARTIAL_LANGUAGE                0x02
 #define MUI_LIP_LANGUAGE                    0x04
 #define MUI_LANGUAGE_INSTALLED              0x20
 #define MUI_LANGUAGE_LICENSED               0x40


 static NTSTATUS get_dummy_preferred_ui_language( DWORD flags, LANGID lang, ULONG *count,
                                                 WCHAR *buffer, ULONG *size )
{
    WCHAR name[LOCALE_NAME_MAX_LENGTH + 2];
    NTSTATUS status;
    ULONG len;

    if (flags & MUI_LANGUAGE_ID) swprintf( name, name, L"%04lX", lang );
    else
    {
        UNICODE_STRING str;

        if (lang == LOCALE_CUSTOM_UNSPECIFIED)
            NtQueryInstallUILanguage( &lang );

        str.Buffer = name;
        str.MaximumLength = sizeof(name);
        status = STATUS_SUCCESS;
       // status = RtlLcidToLocaleName( lang, &str, 0, FALSE );
        if (status) return status;
    }

    len = wcslen( name ) + 2;
    name[len - 1] = 0;
    if (buffer)
    {
        if (len > *size)
        {
            *size = len;
            return STATUS_BUFFER_TOO_SMALL;
        }
        memcpy( buffer, name, len * sizeof(WCHAR) );
    }
    *size = len;
    *count = 1;
    //TRACE("returned variable content: %ld, \"%s\", %ld\n", *count, debugstr_w(buffer), *size);
    return STATUS_SUCCESS;

}

NTSTATUS WINAPI RtlGetUserPreferredUILanguages( DWORD flags, ULONG unknown, ULONG *count,
                                                WCHAR *buffer, ULONG *size )
{
    LANGID ui_language;

    if (flags & ~(MUI_LANGUAGE_NAME | MUI_LANGUAGE_ID)) return STATUS_INVALID_PARAMETER;
    if ((flags & MUI_LANGUAGE_NAME) && (flags & MUI_LANGUAGE_ID)) return STATUS_INVALID_PARAMETER;
    if (*size && !buffer) return STATUS_INVALID_PARAMETER;

    NtQueryDefaultUILanguage( &ui_language );
    return get_dummy_preferred_ui_language( flags, ui_language, count, buffer, size );
}


/* EOF */
