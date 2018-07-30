# Known issues and bugs

## Troubleshooting

In general, if you experience odd or erroneous behavior, you should try building
from source with debug options enabled. This can be achieved by navigating to
the root directory and issuing these commands:
```
make clean
./configure
make
```

## Known issues

All issues have varying degrees of badness. An issue may have a workaround, but
depends on the symptoms and the source of the issue.

Symptom                  | Cause                                     | Solution
-------------------------|-------------------------------------------|-------------------------------------------------------
Ugly user interface font | The original game font could not be found | No workaround at the moment
"    "    "         "    | Or: rendered font looks misplaced         | "  "          "  "   "
No music                 | The CD-ROM audio partition is not mounted | Open the directory in your filebrowser to automount it
Cannot patch DRS bitmap  | The bitmap header is also included        | Strip the header manually

In case the game does not start at all, make sure the following is satisfied:

* The game is not run as root.
* The game is marked as executable (`chmod +x ./game/aoe`)

## Reporting bugs
Remember that at the moment this project is very unstable and lacks lots of
functionality compared to the original game. Before reporting bugs make sure the
following conditions are satisfied:

### Clean build
Always build from scratch (i.e. run `make clean && make') and make sure your
problem is reproducable.

### Bug description
Provide a description of the bug and how it is triggered. If it is not
reproducable on your site, it probably is not on our side either. You have to
provide all output that is printed when running `./empx -v'

### Unique bug
Duplicates or similar bugs that are already reported are ignored. If you are
sure it is not a duplicate you have to tell why you think it is unique.


Example
This is an example of a good bug report:

```
Subject: Failure to Build: No rule to make target '../genie/libgenie.a'

Doesn't seem to build anymore (from a clean sync and build)...
make[1]: *** No rule to make target '../genie/libgenie.a', needed by 'cheat'. Stop.

I assume it's related to bff4917
```

Use your common sense when filing a bug report. This will save time for both
parties. If you are really willing to help improve development you can fix the
bugs yourself and submit a pull request or patch so we can apply the changes. It
goes without saying that you provide a good description *why* we should merge a
particular contribution.

See also [HACKING](HACKING.md) for more information about contributing to this project.
