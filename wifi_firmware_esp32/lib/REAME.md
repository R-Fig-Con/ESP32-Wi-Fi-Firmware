# Shared code directory

As seen in [this link](https://docs.platformio.org/en/latest/advanced/unit-testing/structure/best-practices.html) code used by
unit testing ought not to be in the src folder. Therefore code also used for unit testing should be placed here.

## Println debug

Project uses Println debug option activated by define MONITOR_DEBUG_MODE. Since this folder does not recognize [include](../include/) folder,
define should either be placed on this folder or the [.ini file](../platformio.ini) build flags should indicate its existance.