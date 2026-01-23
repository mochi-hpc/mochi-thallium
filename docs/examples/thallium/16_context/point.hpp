#include <iostream>
#include <sstream>

class point {

    private:

        double x;
        double y;
        double z;

    public:

        point(double a=0.0, double b=0.0, double c=0.0)
            : x(a), y(b), z(c) {}

        template<typename A> friend void serialize(A& ar, point& p);

        double operator*(const point& p) const {
            return p.x * x + p.y * y + p.z * z;
        }

        std::string to_string() const {
            std::stringstream ss;
            ss << "(" << x << "," << y << "," << z << ")";
            return ss.str();
        }

        point operator+(const point& p) const {
            return point{p.x + x, p.y + y, p.z + z};
        }
};

template<typename A>
void serialize(A& ar, point& p) {
    point& q = std::get<0>(ar.get_context());
    double d = std::get<1>(ar.get_context());
    std::cout << "Serializing with context q = "
              << q.to_string()
              << " and d = "
              << d << std::endl;
    ar & (p.x + q.x)*d;
    ar & (p.y + q.y)*d;
    ar & (p.z + q.z)*d;
}

