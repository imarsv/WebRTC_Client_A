`
gclient sync
`


`
gn clean out/Default
`


gn gen out/Default --args='is_clang=false treat_warnings_as_errors=false rtc_include_tests=false'

gn gen out/Default --args='is_clang=false treat_warnings_as_errors=false rtc_include_tests=false use_custom_libcxx=false use_custom_libcxx_for_host=false'

gn gen out/Default --args='is_clang=false treat_warnings_as_errors=false rtc_include_tests=false use_custom_libcxx=false use_custom_libcxx_for_host=false use_ozone=true rtc_include_pulse_audio=false use_rtti=true'

gn gen out/Default --args='is_clang=false treat_warnings_as_errors=false rtc_include_tests=false use_custom_libcxx=false use_custom_libcxx_for_host=false use_ozone=true rtc_include_pulse_audio=false use_rtti=true rtc_build_json=false'


`
ninja -C out/Default
`

`
ninja -C out/Default -t compdb c cc cxx asm objc objcxx alink solink solink_module link > compile_commands.json
`