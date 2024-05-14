== matix sum: =======
// input:  >a[][]=b[][] + c[][]<
for(int i_0 = 0; i_0 < k_size(a, 0); i_0++)                            // line: [ 2]
for(int i_1 = 0; i_1 < k_size(a, 1); i_1++)                            // line: [ 2]
	a[i_0][i_1]=b[i_0][i_1]+c[i_0][i_1]


== matrix multiply: ======
// input:  >a[][] = b[][t:] * c[t] []<
for(int i_0 = 0; i_0 < k_size(a, 0); i_0++)                            // line: [ 6]
for(int i_1 = 0; i_1 < k_size(a, 1); i_1++)                            // line: [ 6]
for(int t = 0; t < k_size(b, 1); t++)                                  // line: [ 6]
	a[i_0][i_1]=b[i_0][t]*c[t][i_1]


== fill with numbers: ======
// input:  >vector[i:][]  = []^2 + 2*[] + a[]<
for(int i = 0; i < k_size(vector, 0); i++)                             // line: [10]
for(int i_1 = 0; i_1 < k_size(vector, 1); i_1++)                       // line: [10]
	vector[i][i_1]=i_1^2+2*i_1+a[i_1]







== nested test: =====
// input:  >a[p[[]]] = c[b[]]<
for(int i_0 = 0; i_0 < 0+1; i_0++)                                     // line: [19]
	a[p[i_0]]=c[b[i_0]]


== bracket is limit in for: =====
// input:  >data[:quantity[]]<
for(int i_0 = 0; i_0 < k_size(quantity, 0); i_0++)                     // line: [23]
for(int i_1 = 0; i_1 < quantity[i_0]; i_1++)                           // line: [23]
	data[i_1]
