#ifndef EXPRESSION_H
#define EXPRESSION_H
#include <cmath>
#include <complex>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <stdexcept>

enum Operation { PLUS, MINUS, MULT, DIV, POW };
enum Function { SIN, COS, LN, EXP };

struct operators {
    Operation type; // тип
    int priority; // приоритет
    explicit operators(const Operation type): type(type) { // конструктор
        switch (type) {
            case PLUS: case MINUS: priority = 1; break;
            case MULT: case DIV: priority = 2; break;
            case POW: priority = 3;
        }
    }
};

inline std::string to_string_optimized(double a) {
    auto a_ = std::floor(a);
    if (a == a_) {
        return std::to_string(static_cast<long>(a_));
    }
    return std::to_string(a);
}

template <typename T>
struct Expression {
    virtual ~Expression() = default;

    virtual std::string to_string() = 0;
    virtual T eval(std::map<std::string, T> &parameters) = 0;
    virtual std::shared_ptr<Expression<T>> diff(std::string &str) = 0;
};

template <typename T>
class ConstantExpression : public Expression<T> {
    T value;
public:
    explicit ConstantExpression(T value) : value(value) {}
    ~ConstantExpression() override = default;
    ConstantExpression(const ConstantExpression<T> &other) = default;
    ConstantExpression(ConstantExpression<T> &&other) = default;
    ConstantExpression &operator=(const ConstantExpression<T> &other) = default;
    ConstantExpression &operator=(ConstantExpression<T> &&other) = default;

    T eval(std::map<std::string, T> &parameters) override {
        return value;
    }
    std::shared_ptr<Expression<T>> diff(std::string &str) override {
        return std::make_shared<ConstantExpression<T>>(T(0));
    }
    std::string to_string() override {
        if constexpr (std::is_same_v<T, std::complex<double>>) {
            // Специальная обработка для комплексных чисел
            double real = value.real();
            double imag = value.imag();
            if (real != 0 && imag != 0) {
                if (imag >= 0) return "(" + to_string_optimized(real) + " + " + to_string_optimized(imag) + "i)";
                return "(" + to_string_optimized(real) + " - " + to_string_optimized(-imag) + "i)";
            }
            if (real == 0 && imag == 0) {
                return to_string_optimized(0);
            }
            if (real == 0) return to_string_optimized(imag) + "i";
            else return to_string_optimized(real);
        } else if (std::is_same_v<T, double>) {
            if (value < 0.0f) return "(" + to_string_optimized(value) + ")";
            return to_string_optimized(value);
        }
    }
};

template <typename T>
class VarExpression : public Expression<T> {
    std::string value;
public:
    explicit VarExpression(const std::string &value) : value(value) {}
    ~VarExpression() override = default;
    VarExpression(const VarExpression<T> &other) = default;
    VarExpression(VarExpression<T> &&other) = default;
    VarExpression &operator=(const VarExpression<T> &other) = default;
    VarExpression &operator=(VarExpression<T> &&other) = default;

    T eval(std::map<std::string, T> &parameters) override {
        return parameters[value];
    }
    std::shared_ptr<Expression<T>> diff(std::string &str) override {
        if (str == value) return std::make_shared<ConstantExpression<T>>(T(1));
        return std::make_shared<ConstantExpression<T>>(T(0));
    }
    std::string to_string() override {
        return value;
    }
};

template <typename T>
class MonoExpression: public Expression<T> {
    std::shared_ptr<Expression<T>> expr;
    Function func;
public:
    MonoExpression(const std::shared_ptr<Expression<T>> &expr, Function func)
        : expr(expr), func(func) {}
    ~MonoExpression() override = default;
    MonoExpression(const MonoExpression<T> &other) = default;
    MonoExpression(MonoExpression<T> &&other) = default;
    MonoExpression &operator=(const MonoExpression<T> &other) = default;
    MonoExpression &operator=(MonoExpression<T> &&other) = default;

    T eval(std::map<std::string, T> &parameters) override {
        switch (func) {
            case SIN: return std::sin(expr->eval(parameters));
            case COS: return std::cos(expr->eval(parameters));
            case LN: return std::log(expr->eval(parameters));
            case EXP: return std::exp(expr->eval(parameters));
            default: throw std::runtime_error("Unknown function");
        }
    }
    std::shared_ptr<Expression<T>> diff(std::string &str) override; // реализация ниже
    std::string to_string() override ;
    friend std::shared_ptr<Expression<T>> optimize<T> (std::shared_ptr<Expression<T>> expr);

};

template <typename T>
class BinaryExpression: public Expression<T> {
    std::shared_ptr<Expression<T>> left;
    std::shared_ptr<Expression<T>> right;
    Operation op;
public:
    BinaryExpression(const std::shared_ptr<Expression<T>> &left,
                     const std::shared_ptr<Expression<T>> &right,
                     Operation op)
        : left(left), right(right), op(op) {
        /*if (auto right_ptr = dynamic_pointer_cast<ConstantExpression<T>>(T(0))) {
            throw std::runtime_error("Division by zero");
        }*/
    }
    ~BinaryExpression() override = default;
    BinaryExpression(const BinaryExpression<T> &other) = default;
    BinaryExpression(BinaryExpression<T> &&other) = default;
    BinaryExpression &operator=(const BinaryExpression<T> &other) = default;
    BinaryExpression &operator=(BinaryExpression<T> &&other) = default;

    T eval(std::map<std::string, T> &parameters) override {
        switch (op) {
            case PLUS: return left->eval(parameters) + right->eval(parameters);
            case MINUS: return left->eval(parameters) - right->eval(parameters);
            case MULT: return left->eval(parameters) * right->eval(parameters);
            case DIV:
                if (right->eval(parameters) == T(0)) throw std::runtime_error("Division by zero");
                return left->eval(parameters) / right->eval(parameters);
            case POW: return std::pow(left->eval(parameters), right->eval(parameters));
            default: throw std::runtime_error("Unknown operation");

        }
    }
    std::shared_ptr<Expression<T>> diff(std::string &str) override {
        auto left_diff = left->diff(str);
        auto right_diff = right->diff(str);
        switch (op) {
            case PLUS:
                return std::make_shared<BinaryExpression<T>>(left_diff, right_diff, PLUS);
            case MINUS:
                return std::make_shared<BinaryExpression<T>>(left_diff, right_diff, MINUS);
            case MULT: {
                auto left_mult = std::make_shared<BinaryExpression<T>>(left_diff, right, MULT);
                auto right_mult = std::make_shared<BinaryExpression<T>>(left, right_diff, MULT);
                return std::make_shared<BinaryExpression<T>>(left_mult, right_mult, PLUS);
            }
            case DIV: {
                auto numerator = std::make_shared<BinaryExpression<T>>(
                    std::make_shared<BinaryExpression<T>>(left_diff, right, MULT),
                    std::make_shared<BinaryExpression<T>>(left, right_diff, MULT),
                    MINUS);
                auto denominator = std::make_shared<BinaryExpression<T>>(
                    right, std::make_shared<ConstantExpression<T>>(T(2)), POW);
                return std::make_shared<BinaryExpression<T>>(numerator, denominator, DIV);
            }
            case POW: {

                if constexpr (!std::is_same_v<T, std::complex<double>>) {
                    std::map<std::string, T> map;
                    // f(x) ^ const
                    if (auto right_const = std::dynamic_pointer_cast<ConstantExpression<T>>(right)) {
                        if (right_const->eval(map) > T(1)) {
                            auto power = std::make_shared<ConstantExpression<T>>(right_const->eval(map) - T(1));
                            auto multiplier = std::make_shared<BinaryExpression<T>>(left, power, POW);
                            return std::make_shared<BinaryExpression<T>>(
                                right,
                                std::make_shared<BinaryExpression<T>>(
                                    multiplier , left_diff, MULT),
                                MULT);
                        }
                        if (right_const->eval(map) == 1)
                            return std::make_shared<ConstantExpression<T>>(T(1));

                        auto multiplier = std::make_shared<BinaryExpression<T>>(right, left_diff, MULT);
                        auto power = std::make_shared<ConstantExpression<T>>(T(std::abs(right_const->eval(map)) + T(1)));
                        return std::make_shared<BinaryExpression<T>>(multiplier,
                            std::make_shared<BinaryExpression<T>>(left, power, POW), DIV);
                    }
                    // const ^ f(x)
                    if (auto left_const = std::dynamic_pointer_cast<ConstantExpression<T>>(left)) {
                        auto multiplier1 = std::make_shared<BinaryExpression<T>>(left, right, POW);
                        auto multiplier2 = std::make_shared<MonoExpression<T>>(left, LN);
                        return std::make_shared<BinaryExpression<T>>(
                            right_diff,
                            std::make_shared<BinaryExpression<T>>(
                                multiplier1,
                                multiplier2, MULT),
                            MULT);
                    }

                    // f(x) ^ g(x)
                    auto term1 = std::make_shared<BinaryExpression<T>>(
                        right_diff, std::make_shared<MonoExpression<T>>(left, LN), MULT);
                    auto term2 = std::make_shared<BinaryExpression<T>>(
                        right, std::make_shared<BinaryExpression<T>>(left_diff, left, DIV), MULT);
                    return std::make_shared<BinaryExpression<T>>(term1, term2, PLUS);
                }

            }
            default: throw std::runtime_error("Unknown operation");
        }
    }
    std::string to_string() override {
        switch (op) {
            case PLUS: return "(" + left->to_string() + " + " + right->to_string() + ")";
            case MINUS: return "(" + left->to_string() + " - " + right->to_string() + ")";
            case MULT: return "(" + left->to_string() + " * " + right->to_string() + ")";
            case DIV: return "(" + left->to_string() + " / " + right->to_string() + ")";
            case POW: return "(" + left->to_string() + "^" + right->to_string() + ")";
            default: return "Unknown operation";
        }
    }
    friend std::shared_ptr<Expression<T>> optimize<T> (std::shared_ptr<Expression<T>> expr);
};
template<typename T>
std::string MonoExpression<T>::to_string() {
    if (auto binary = std::dynamic_pointer_cast<BinaryExpression<T>>(expr)) {
        switch (func) {
            case SIN: return "sin" + expr->to_string();
            case COS: return "cos" + expr->to_string();
            case LN: return "ln" + expr->to_string();
            case EXP: return "exp" + expr->to_string();
            default: return "Unknown function";
        }
    }
    switch (func) {
        case SIN: return "sin(" + expr->to_string() + ")";
        case COS: return "cos(" + expr->to_string() + ")";
        case LN: return "ln(" + expr->to_string() + ")";
        case EXP: return "exp(" + expr->to_string() + ")";
        default: return "Unknown function";
    }
}

template<typename T>
std::shared_ptr<Expression<T> > MonoExpression<T>::diff(std::string &str) {
    auto expr_diff = expr->diff(str);
    switch (func) {
        case SIN:
            return std::make_shared<BinaryExpression<T>>(
                std::make_shared<MonoExpression<T>>(expr, COS),
                expr_diff, MULT);
        case COS:
            return std::make_shared<BinaryExpression<T>>(
                std::make_shared<BinaryExpression<T>>(
                    std::make_shared<ConstantExpression<T>>(T(-1)),
                    std::make_shared<MonoExpression<T>>(expr, SIN),
                    MULT),
                expr_diff, MULT);
        case LN:
            return std::make_shared<BinaryExpression<T>>(expr_diff, expr, DIV);
        case EXP:
            return std::make_shared<BinaryExpression<T>>(
                std::make_shared<MonoExpression<T>>(expr, EXP),
                expr_diff, MULT);
        default: throw std::runtime_error("Unknown function");
    }
}

template <typename T>
std::shared_ptr<Expression<T>> optimize (std::shared_ptr<Expression<T>> expr) {
    if (auto mono = std::dynamic_pointer_cast<MonoExpression<T>>(expr)) {
        mono->expr = optimize(mono->expr);
    }
    if (auto binary = std::dynamic_pointer_cast<BinaryExpression<T>>(expr)) {
        binary->left = optimize(binary->left);
        binary->right = optimize(binary->right);
        /*if (auto left_b = std::dynamic_pointer_cast<BinaryExpression<T>>(binary->left)) {
            optimize(std::dynamic_pointer_cast<Expression<T>>(left_b));
        }
        if (auto right_b = std::dynamic_pointer_cast<BinaryExpression<T>>(binary->right)) {
            optimize(std::dynamic_pointer_cast<Expression<T>>(right_b));
        }*/
        auto left = std::dynamic_pointer_cast<ConstantExpression<T>>(binary->left);
        auto right = std::dynamic_pointer_cast<ConstantExpression<T>>(binary->right);
        // Если сложение или вычитание нас интересуют нули
        if (binary->op == PLUS || binary->op == MINUS) {
            // Оба константы
            if (left && right) {
                std::map <std::string, T> map;
                // Есть ноль
                if (left->eval(map) == T(0) || right->eval(map) == T(0)) {
                    expr = std::make_shared<ConstantExpression<T>>(T(expr->eval(map)));
                }
            // Если только левое выражение - константа
            } else if (left) {
                std::map <std::string, T> map;
                // Если оно ноль
                if (left->eval(map) == T(0)) {
                    if (binary->op == MINUS) {
                        binary->op = MULT;
                        binary->left = std::make_shared<ConstantExpression<T>>(T(-1));
                    } else {
                        expr = binary->right;
                    }
                }
            // Если только правое выражение - константа
            } else if (right) {
                std::map <std::string, T> map;
                // Если оно ноль
                if (right->eval(map) == T(0)) {
                    expr = binary->left;
                }
            }
        }
        if (binary->op == MULT || binary->op == DIV) {
            // Оба константы
            if (left && right) {
                std::map <std::string, T> map;
                // Есть ноль
                if (left->eval(map) == T(0) || right->eval(map) == T(0)) {
                    if (binary->op == MULT) {
                        expr = std::make_shared<ConstantExpression<T>>(T(0));
                    } else if (binary->op == DIV) {
                        if (right->eval(map) == T(0)) {
                            throw std::runtime_error("Division by zero");
                        } else if (left->eval(map) == T(0)) {
                            expr = std::make_shared<ConstantExpression<T>>(T(0));
                        }
                    }
                }
                // Есть единица
                if (left->eval(map) == T(1) || right->eval(map) == T(1)) {
                    expr = std::make_shared<ConstantExpression<T>>(T(expr->eval(map)));
                }
            // Если только левое выражение - константа
            } else if (left) {
                std::map <std::string, T> map;
                // Если оно единица
                if (left->eval(map) == T(1)) {
                    if (binary->op == MULT) {
                        expr = binary->right;
                    }
                }
                // Если - 0
                if (left->eval(map) == T(0)) {
                    expr = std::make_shared<ConstantExpression<T>>(T(0));
                }
                // Если только правое выражение - константа
            } else if (right) {
                std::map <std::string, T> map;
                // Если оно единица
                if (right->eval(map) == T(1)) {
                    expr = binary->left;
                }
                // Если - 0
                if (right->eval(map) == T(0)) {
                    expr = std::make_shared<ConstantExpression<T>>(T(0));
                }
            }
        } //
    }
    return expr;
}

#endif //EXPRESSION_H
