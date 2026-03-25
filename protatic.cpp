#include<iostream>
#include<string>
using namespace std;
int main(){
    string s;
    cin>>s;
    char a[]="hello";
    int ans=0;
    for(int i=0;i<s.size();i++){
        if(s[i]==a[i]) ans++;
    }
    if(ans=5)cout<<"YES";
    else cout<<"NO";
    return 0;
}