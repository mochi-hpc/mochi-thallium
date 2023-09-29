class point {

    private:

        double x;
        double y;
        double z;

    public:

        point(double a=0.0, double b=0.0, double c=0.0)
            : x(a), y(b), z(c) {}

        point(const point&) = delete;

        template<typename A> friend void serialize(A& ar, point& p);

        double operator*(const point& p) const {
            return p.x * x + p.y * y + p.z * z;
        }
};

template<typename A>
void serialize(A& ar, point& p) {
    ar & p.x;
    ar & p.y;
    ar & p.z;
}
