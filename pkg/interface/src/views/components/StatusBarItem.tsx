import React, { ReactNode } from "react";
import { Row as _Row, Icon, Button } from "@tlon/indigo-react";
import styled from "styled-components";

const Row = styled(_Row)`
  cursor: pointer;
`;

type StatusBarItemProps = Parameters<typeof Row>[0] & { badge?: boolean };

export function StatusBarItem({
  badge,
  children,
  ...props
}: StatusBarItemProps) {
  return (
    <Button
      position="relative"
      border={1}
      color="washedGray"
      bg="white"
      px={2}
      {...props}
    >
      {children}
      {badge && (
        <Icon
          size="22px"
          icon="Bullet"
          color="blue"
          style={{ position: 'absolute', top: '-10', right: '-11' }}
        />
      )}
    </Button>
  );
}
