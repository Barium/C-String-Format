#include <iostream>
#include <map>
#include <vector>

#include <utils/format.h>

using namespace std;
using namespace utils::str;

namespace {

void BeginTest(int number, const char* description)
{
    cout << endl
         << "Test case #" << number << ":" << endl
         << description << endl
         << endl;
}

}

int main(int argc, char* argv[])
{
    cout << "Type safe string format" << endl
         << endl
         << "By Tommy Andersen" << endl
         << "http://www.tommya.net" << endl
         << "https://bitbucket.org/tommyandersen/templated-c-string-format" << endl
         << endl
         << "Testing string format methods." << endl;

    int testIndex = 1;

    BeginTest(testIndex++, "Hello World, the simple example with a single parameter.");
    cout << "  Format(\"Hello {}\", \"World\") =>" << endl;
    cout << "  " << Format("Hello {}", "World") << endl;

    BeginTest(testIndex++, "Displaying a number of integers, using simple referencing.");
    cout << "  Format(\"{}, {}, {}, {}, {}\", 1, 2, 3, 4, 5) =>" << endl;
    cout << "  " << Format("{}, {}, {}, {}, {}", 1, 2, 3, 4, 5) << endl;

    BeginTest(testIndex++, "Displaying a number of integers, using specific index referencing.");
    cout << "  Format(\"{4}, {3}, {2}, {1}, {0}\", 1, 2, 3, 4, 5) =>" << endl;
    cout << "  " << Format("{4}, {3}, {2}, {1}, {0}", 1, 2, 3, 4, 5) << endl;

    BeginTest(testIndex++, "Referencing the same element multiple times.");
    cout << "  Format(\"{0}, {0}, {0}, {1}, {0}\", 1, 2) =>" << endl;
    cout << "  " << Format("{0}, {0}, {0}, {1}, {0}", 1, 2) << endl;

    BeginTest(testIndex++, "Referencing different types.");
    cout << "  string testStr = \"std::string\";" << endl;
    cout << "  Format(\"{}, {}, {}, {}, {}\", 10, 2.5, true, \"char ptr\", testStr) =>" << endl;
    string testStr = "std::string";
    cout << "  " << Format("{}, {}, {}, {}, {}", 10, 2.5, true, "char ptr", testStr) << endl;

    BeginTest(testIndex++, "Setting padding, width, and alignment on parameters.");
    cout << "  Format(\"'{0:05}', '{0:5}', '{0:<5}', '{0:>5}', '{0:^5}'\", 1) =>" << endl;
    cout << "  " << Format("'{0:05}', '{0:5}', '{0:<5}', '{0:>5}', '{0:^5}'", 1) << endl;

    BeginTest(testIndex++, "Setting precision, width and alignment.");
    cout << "  Format(\"{0:.2}, {0:.0}, {0:05.3}, {0:.5}, {0:<010.10}\", 2.12579) =>" << endl;
    cout << "  " << Format("{0:.2}, {0:.0}, {0:05.3}, {0:.5}, {0:<010.10}", 2.12579) << endl;

    BeginTest(testIndex++, "Using vectors.");
    cout << "  vector<int> testVec = {1, 2, 3, 4, 5};" << endl;
    cout << "  map<string, double> testMap = {{\"1\", 1.5}, {\"2\", 3.0}, {\"3\", 4.5}};" << endl;
    cout << "  Format(\"{}, {}\", testVec, testMap) =>" << endl;
    vector<int> testVec = {1, 2, 3, 4, 5};
    map<string, double> testMap = {{"1", 1.5}, {"2", 3.0}, {"3", 4.5}};
    cout << "  " << Format("{}, {}", testVec, testMap) << endl;

    BeginTest(testIndex++, "Referencing specific indexes and keys in vectors and maps.");
    cout << "  Format(\"{0.0}, {0[2]}, {0[4]}\", testVec, testMap) =>" << endl;
    cout << "  " << Format("{0.1}, {0[2]}, {0[1]}", testMap) << endl;
    return 0;
}
