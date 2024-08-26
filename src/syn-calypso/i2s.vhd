library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity i2s is
  port (
    clock : in std_logic;
    i2s   : out std_logic_vector(2 downto 0);
    l     : in std_logic_vector(15 downto 0);
    r     : in std_logic_vector(15 downto 0)
  );
end entity i2s;

architecture rtl of i2s is
  signal ce : unsigned(8 downto 0);
  signal ce4, ce5, ce9a, ce9b : std_logic;
  signal ck, lr, q : std_logic;
  signal sr : std_logic_vector(14 downto 0);
begin

  process(clock)
  begin
    if rising_edge(clock) then
      ce <= ce + 1;
    end if;
  end process;

  ce4 <= '1' when ce(3 downto 0) = "1111" else '0';
  ce5 <= '1' when ce(4 downto 0) = "11111" else '0';
  ce9a <= '1' when ce = "111111111" else '0';
  ce9b <= '1' when ce(4 downto 0) /= "11111" else '0';

  process(clock)
  begin
    if rising_edge(clock) then
      if ce4 then
        ck <= not ck;
      end if;
    end if;
  end process;

  process(clock)
  begin
    if rising_edge(clock) then
      if ce9b then
        lr <= not lr;
      end if;
    end if;
  end process;

  process(clock)
  begin
    if rising_edge(clock) then
      if ce9a then
        if lr = '1' then
          sr <= r & '0';
        else
          sr <= l & '0';
        end if;
        q <= sr(14);
      elsif ce5 then
        sr <= '0' & sr(14 downto 1);
      end if;
    end if;
  end process;

  i2s <= ck & lr & q;

end architecture rtl;