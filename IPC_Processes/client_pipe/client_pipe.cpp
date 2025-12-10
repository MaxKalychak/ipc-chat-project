#include <iostream>
int main(int argc, char* argv[])
{
    std::cout << "[client_pipe] Started. Method=" << (argc > 1 ? argv[1] : "?") << "\n";
    system("pause");
    return 0;
}
