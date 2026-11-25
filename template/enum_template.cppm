export module template:enum_template;

export namespace template{
    //enum class bertipe type dan flags diawali eNone dan eMaxEnum
    enum class EnumTemplateTypes{
        //buat prefiks e... kemudian sejajarkan = dan nilai hexagonal
        eNone           = 0x00000000,
        eOne            = 0x00000001,
        eAnother        = 0x00000002,
        eOthers         = 0x00000003,
        eMaxEnum        = 0xFFFFFFFF
    };
    enum class EnumTemplateFlags{
        eNone           = 0x00000000,
        eOne            = 0x00000001,
        eAnother        = 0x00000002,
        eOthers         = 0x00000004,
        eMaxEnum        = 0xFFFFFFFF
    };
    constexpr EnumTemlateFlags operator|(EnumTemlateFlags lhs, EnumTemlateFlags rhs) {
        using T = std::underlying_type_t<EnumTemlateFlags>;
        return static_cast<EnumTemlateFlags>(static_cast<T>(lhs) | static_cast<T>(rhs));
    }

    // Operator AND (&)
    constexpr EnumTemlateFlags operator&(EnumTemlateFlags lhs, EnumTemlateFlags rhs) {
        using T = std::underlying_type_t<EnumTemlateFlags>;
        return static_cast<EnumTemlateFlags>(static_cast<T>(lhs) & static_cast<T>(rhs));
    }

    // Operator OR Assignment (|=)
    constexpr EnumTemlateFlags& operator|=(EnumTemlateFlags& lhs, EnumTemlateFlags rhs) {
        lhs = lhs | rhs;
        return lhs;
    }

}
