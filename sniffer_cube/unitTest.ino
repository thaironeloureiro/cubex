#line 2 "unitTest.ino"
#ifdef UNIT_TEST

test(getPath)
{
  char prev_test[MAXNODES];  
  prev_test[54]=53;//Dada a posição 54, retorna a posição anterior num caminho calculado.
  prev_test[56]=0; //Dada a posição 56, não há posição anterior para chegar nessa celula - indicando que não há caminho possivel
  
  tem_rota=false;
  tem_rota = getPath(54, prev_test);
  assertEqual(tem_rota, true);

  tem_rota = getPath(56, prev_test);
  assertNotEqual(tem_rota, true);
}

#endif
