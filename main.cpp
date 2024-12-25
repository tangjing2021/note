#include <iostream>
#define ll long long
const int mx_int=1e7;
const int time_int=10;
ll sum[mx_int];
ll fun_pow(ll a, ll b,ll gcd) {
    ll ans=1;
    a%=gcd;
    while (b) {
        if ((b&1)) {
            ans=(ans*a)%gcd;
        }
        b>>=1;
        a=(a*a)%gcd;
    }
    return ans;
}

void test1() {
    for (ll x=0;x<mx_int;x++) {
        sum[x]=x;
    }
}
void test2() {
    for (ll x=1;x<mx_int;x++) {
        sum[x]=sum[x]+sum[x-1];
    }
}
void test3() {
    sum[0]=1;
    sum[1]=2;
    for (ll x=2;x<mx_int;x++) {
        sum[x]=sum[x-1]*sum[x-2];
    }
}
void test4() {
    for (ll x=0;x<mx_int;x++) {
        sum[x]=fun_pow(x,x,1e9+7);
    }
}
void test5() {
    for (ll x : sum) {
        printf("%lld\n",x);
    }
}

void solve() {
    test1();
    test2();
    test3();
    test4();
    test5();
}

int main()
{
    FILE *f1=fopen("ks.txt","w");
    fclose(f1);
    for (int i=0;i<time_int;i++) {
        solve();
    }
    FILE *f2=fopen("js.txt","w");
    fclose(f2);
    return 0;
}
