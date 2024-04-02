#include "std_lib_facilities.h"


const char number = '8';    // t.kind==number 表示 t 是一个数字型Token
const char quit   = 'q';    // t.kind==quit means 表示 t 是退出型Token
const char print  = ';';    // t.kind==print means that t 是打印型Token
const char name   = 'a';    // 变量名Token
const char let    = 'L';    // 定义变量token
const string declkey = "let";// 定义变量的关键字
const string prompt  = "> "; // 输入提示符
const string result  = "= "; // 输出提示符



class Token {
public:
    char kind;        // token的类型
    double value;     // for numbers: token的值
    string name;      // for names: token的名称
    Token(char ch)             : kind(ch), value(0)   {}
    Token(char ch, double val) : kind(ch), value(val) {}
    Token(char ch, string n)   : kind(ch), name(n)    {}
};


class Token_stream {
public: 
    Token_stream();   // 从cin中得到token流
    Token get();      // 得到token
    void putback(Token t);    // 把token放回流中
    void ignore(char c);      // 错误恢复
private:
    bool full;
    Token buffer;
};


// 初始化token_stream
Token_stream::Token_stream()
:full(false), buffer(0)    
{
}

void Token_stream::putback(Token t)
{
    if (full) error("putback() into a full buffer");
    buffer = t;       
    full = true;      
}

// 从标准输入中读字符并组成 Token
Token Token_stream::get()
{
    if (full) {         // check if we already have a Token ready
        full=false;
        return buffer;
    }  

    char ch;
    cin >> ch;          // note that >> skips whitespace (space, newline, tab, etc.)

    switch (ch) {
    case quit:
    case print:
    case '(':
    case ')':
    case '+':
    case '-':
    case '*':
    case '/': 
    case '%':
    case '=':
        return Token(ch); // let each character represent itself
    case '.':             // a floating-point literal can start with a dot
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':    // numeric literal
    {
        cin.putback(ch);// put digit back into the input stream
        double val;
        cin >> val;     // read a floating-point number
        return Token(number,val);
    }
    default:
        if (isalpha(ch)) {
            string s;
            s += ch;
            while (cin.get(ch) && (isalpha(ch) || isdigit(ch))) s+=ch;
            cin.putback(ch);
            if (s == declkey) return Token(let); // keyword "let"
            return Token(name,s);
        }
        error("Bad token");
    }
}

// 如果缓冲区中的字符是c就丢掉并返回，否则一直从cin中读取直到遇见c
// 目的是处理一行中有多个输入，其中有错误的输入，以从错误中恢复
void Token_stream::ignore(char c)
{
    // first look in buffer:
    if (full && c==buffer.kind) {
        full = false;
        return;
    }
    full = false;

    // now search input:
    char ch = 0;
    while (cin>>ch)
        if (ch==c) return;
}


Token_stream ts;


class Variable {
public:
    string name;
    double value;
    Variable (string n, double v) :name(n), value(v) { }
};


vector<Variable> var_table;

// 返回名为 s 的变量的值
double get_value(string s)
{
    for (int i = 0; i<var_table.size(); ++i)
        if (var_table[i].name == s) return var_table[i].value;
    error("get: undefined variable ", s);
}

// 设置名为 s 的变量的值为 d
void set_value(string s, double d)
{
    for (int i = 0; i<var_table.size(); ++i)
        if (var_table[i].name == s) {
            var_table[i].value = d;
            return;
        }
    error("set: undefined variable ", s);
}

// 判断名为 var 的变量是否已经声明
bool is_declared(string var)
{
    for (int i = 0; i<var_table.size(); ++i)
        if (var_table[i].name == var) return true;
    return false;
}

// 声明变量 var, 赋值为 val
double define_name(string var, double val)
{
    if (is_declared(var)) error(var," declared twice");
    var_table.push_back(Variable(var,val));
    return val;
}

// 函数声明
double expression();    


double primary()
{
    Token t = ts.get();
    switch (t.kind) {
    case '(':           // 处理 '(' expression ')'
        {
            double d = expression();
            t = ts.get();
            if (t.kind != ')') error("')' expected");
            return d;
        }
    case number:    
        return t.value;    
    case name:
        return get_value(t.name); 
    // 处理负数
    case '-':
        return - primary();
    case '+':
        return primary();
    default:
        error("primary expected");
    }
}



double term()
{
    double left = primary();
    Token t = ts.get(); // 从单词流中获取单词

    while(true) {
        switch (t.kind) {
        case '*':
            left *= primary();
            t = ts.get();
            break;
        case '/':
            {    
                double d = primary();
                if (d == 0) error("divide by zero");
                left /= d; 
                t = ts.get();
                break;
            }
        // 实现模运算
        case '%':
            {    
                int i1 = narrow_cast<int>(left);
                int i2 = narrow_cast<int>(term());
                if (i2 == 0) error("%: divide by zero");
                left = i1%i2; 
                t = ts.get();
                break;
            }
        default: 
            ts.putback(t);        // 把 t 放回单词流中
            return left;
        }
    }
}



double expression()
{
    double left = term();      
    Token t = ts.get();        

    while(true) {    
        switch(t.kind) {
        case '+':
            left += term();    
            t = ts.get();
            break;
        case '-':
            left -= term();    
            t = ts.get();
            break;
        default: 
            ts.putback(t);
            return left;
        }
    }
}

// 处理变量赋值操作
double declaration()
{
    Token t = ts.get();
    if (t.kind != name) error ("name expected in declaration");
    string var_name = t.name;

    Token t2 = ts.get();
    if (t2.kind != '=') error("= missing in declaration of ", var_name);

    double d = expression();
    define_name(var_name,d);
    return d;
}

// 变量声明语句
double statement()
{
    Token t = ts.get();
    switch (t.kind) {
    case let:
        return declaration();
    default:
        ts.putback(t);
        return expression();
    }
}

// 错误恢复
void clean_up_mess()
{ 
    ts.ignore(print);
}

// 循环计算表达式
void calculate()
{
    while (cin)
      try {
        cout << prompt;                    // 输入提示符
        Token t = ts.get();
        // 可以实现; q的连续判断
        while (t.kind == print) t=ts.get();// first discard all "prints"
        if (t.kind == quit) return;        // 退出程序
        ts.putback(t);
        cout << result << statement() << endl;
    }
    catch (exception& e) {
        cerr << e.what() << endl;          // 输出错误信息
        clean_up_mess();
    }
}

// 程序整体框架:启动程序, 结束程序, 处理致命错误
int main()
try {
    // 预先定义的变量
    define_name("pi",3.1415926535);
    define_name("e",2.7182818284);

    calculate();

    keep_window_open();    // cope with Windows console mode
    return 0;
}
catch (exception& e) {
    cerr << e.what() << endl;
    keep_window_open("~~");
    return 1;
}
catch (...) {
    cerr << "exception \n";
    keep_window_open("~~");
    return 2;
}

